#include "course_plan_core.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <queue>
#include <sstream>
#include <utility>

namespace courseplan {

int CoursePlan::indexOf(const std::string& id) const {
    for (int i = 0; i < static_cast<int>(courses_.size()); ++i) {
        if (courses_[i].id == id) return i;
    }
    return -1;
}

bool CoursePlan::addCourse(const Course& course) {
    if (course.id.empty() || course.name.empty() || course.credits < 0 || indexOf(course.id) >= 0)
        return false;
    courses_.push_back(course);
    edges_.emplace_back();
    return true;
}

bool CoursePlan::updateCourse(const std::string& id, const std::string& name, int credits) {
    const int index = indexOf(id);
    if (index < 0 || name.empty() || credits < 0) return false;
    courses_[index].name = name;
    courses_[index].credits = credits;
    return true;
}

bool CoursePlan::addPrerequisite(const std::string& before, const std::string& after) {
    const int from = indexOf(before);
    const int to = indexOf(after);
    if (from < 0 || to < 0 || from == to) return false;

    auto& outgoing = edges_[from];
    if (std::find(outgoing.begin(), outgoing.end(), to) == outgoing.end()) outgoing.push_back(to);
    return true;
}

bool CoursePlan::removePrerequisite(const std::string& before, const std::string& after) {
    const int from = indexOf(before);
    const int to = indexOf(after);
    if (from < 0 || to < 0) return false;

    auto& outgoing = edges_[from];
    const auto edge = std::find(outgoing.begin(), outgoing.end(), to);
    if (edge == outgoing.end()) return false;
    outgoing.erase(edge);
    return true;
}

bool CoursePlan::removeCourse(const std::string& id) {
    const int removed = indexOf(id);
    if (removed < 0) return false;

    courses_.erase(courses_.begin() + removed);
    edges_.erase(edges_.begin() + removed);
    for (auto& outgoing : edges_) {
        outgoing.erase(std::remove(outgoing.begin(), outgoing.end(), removed), outgoing.end());
        for (int& target : outgoing) {
            if (target > removed) --target;
        }
    }
    return true;
}

TopologyResult CoursePlan::topologicalOrder() const {
    TopologyResult result;
    std::vector<int> indegree(courses_.size(), 0);
    for (const auto& outgoing : edges_) {
        for (int target : outgoing) ++indegree[target];
    }

    std::queue<int> ready;
    for (int i = 0; i < static_cast<int>(courses_.size()); ++i) {
        if (indegree[i] == 0) ready.push(i);
    }

    while (!ready.empty()) {
        const int current = ready.front();
        ready.pop();
        result.order.push_back(current);
        for (int target : edges_[current]) {
            if (--indegree[target] == 0) ready.push(target);
        }
    }

    if (result.order.size() == courses_.size()) return result;

    for (int i = 0; i < static_cast<int>(indegree.size()); ++i) {
        if (indegree[i] > 0) result.unresolved.push_back(i);
    }

    // The unprocessed vertices include the cycle and its downstream courses.
    // DFS identifies one real cycle so the caller can show the conflicting path.
    std::vector<int> color(courses_.size(), 0);
    std::vector<int> parent(courses_.size(), -1);
    const auto findCycle = [&](auto&& self, int current) -> bool {
        color[current] = 1;
        for (int target : edges_[current]) {
            if (indegree[target] == 0) continue;
            if (color[target] == 0) {
                parent[target] = current;
                if (self(self, target)) return true;
            } else if (color[target] == 1) {
                result.cycle.push_back(target);
                for (int vertex = current; vertex != target; vertex = parent[vertex])
                    result.cycle.push_back(vertex);
                result.cycle.push_back(target);
                std::reverse(result.cycle.begin(), result.cycle.end());
                return true;
            }
        }
        color[current] = 2;
        return false;
    };

    for (int i = 0; i < static_cast<int>(courses_.size()) && result.cycle.empty(); ++i) {
        if (indegree[i] > 0 && color[i] == 0) findCycle(findCycle, i);
    }
    return result;
}

SemesterPlan CoursePlan::buildSemesters(int creditLimit) const {
    SemesterPlan result;
    if (creditLimit < 0) {
        result.validCreditLimit = false;
        return result;
    }

    const TopologyResult topology = topologicalOrder();
    result.cycle = topology.cycle;
    result.unresolved = topology.unresolved;

    std::vector<int> indegree(courses_.size(), 0);
    for (const auto& outgoing : edges_) {
        for (int target : outgoing) ++indegree[target];
    }

    std::vector<int> ready;
    for (int i = 0; i < static_cast<int>(courses_.size()); ++i) {
        if (indegree[i] == 0) ready.push_back(i);
    }

    while (!ready.empty()) {
        std::vector<int> semester;
        std::vector<int> deferred;
        int semesterCredits = 0;

        for (int courseIndex : ready) {
            const int credits = courses_[courseIndex].credits;
            if (creditLimit == 0 || semesterCredits + credits <= creditLimit) {
                semester.push_back(courseIndex);
                semesterCredits += credits;
            } else {
                deferred.push_back(courseIndex);
            }
        }

        if (semester.empty()) {
            result.overLimit = std::move(deferred);
            return result;
        }

        result.semesters.push_back(semester);
        std::vector<int> newlyReady;
        for (int courseIndex : semester) {
            for (int target : edges_[courseIndex]) {
                if (--indegree[target] == 0) newlyReady.push_back(target);
            }
        }
        deferred.insert(deferred.end(), newlyReady.begin(), newlyReady.end());
        ready = std::move(deferred);
    }

    return result;
}

bool CoursePlan::load(const std::string& path, std::string* error) {
    const auto fail = [error](std::size_t line, const std::string& reason) {
        if (error) {
            *error = line == 0 ? reason : "第 " + std::to_string(line) + " 行：" + reason;
        }
        return false;
    };

    std::ifstream input(path, std::ios::binary);
    if (!input) return fail(0, "无法打开文件");

    CoursePlan temporary;
    struct PendingEdge {
        std::string before;
        std::string after;
        std::size_t line;
    };
    std::vector<PendingEdge> pendingEdges;
    std::string line;
    std::size_t lineNumber = 0;

    // Defer edges so a file may list them before their course records.
    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3); // The browser export writes a UTF-8 BOM.
        }

        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;

        std::istringstream record(line.substr(first));
        std::string type, a, b, extra;
        if (!(record >> type)) continue;

        if (type == "COURSE") {
            int credits = 0;
            if (!(record >> a >> b >> credits) || (record >> extra))
                return fail(lineNumber, "课程格式应为 COURSE 编号 名称 学分");
            if (!temporary.addCourse({a, b, credits}))
                return fail(lineNumber, "课程编号重复，或课程字段无效");
        } else if (type == "EDGE") {
            if (!(record >> a >> b) || (record >> extra))
                return fail(lineNumber, "先修关系格式应为 EDGE 先修课编号 后续课编号");
            pendingEdges.push_back({a, b, lineNumber});
        } else {
            return fail(lineNumber, "无法识别记录类型：" + type);
        }
    }
    if (input.bad()) return fail(lineNumber, "读取文件时发生错误");

    for (const auto& edge : pendingEdges) {
        if (!temporary.addPrerequisite(edge.before, edge.after))
            return fail(edge.line, "先修关系引用了不存在的课程，或课程不能依赖自身");
    }

    *this = std::move(temporary);
    if (error) error->clear();
    return true;
}

bool CoursePlan::save(const std::string& path) const {
    std::ofstream output(path);
    if (!output) return false;

    output << "# Course plan format: COURSE id name credits; EDGE prerequisite_id course_id\n";
    for (const auto& course : courses_)
        output << "COURSE " << course.id << ' ' << course.name << ' ' << course.credits << '\n';
    for (int from = 0; from < static_cast<int>(courses_.size()); ++from) {
        for (int to : edges_[from])
            output << "EDGE " << courses_[from].id << ' ' << courses_[to].id << '\n';
    }
    return static_cast<bool>(output);
}

} // namespace courseplan
