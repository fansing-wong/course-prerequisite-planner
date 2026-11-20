#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Course prerequisite planner: an AOV network stored as an adjacency list.
// Edge u -> v means course u must be completed before course v.
struct Course {
    std::string id;
    std::string name;
    int credits = 0;
};

class CoursePlan {
    std::vector<Course> courses;
    std::vector<std::vector<int>> edges;

    int indexOf(const std::string& id) const {
        for (int i = 0; i < static_cast<int>(courses.size()); ++i)
            if (courses[i].id == id) return i;
        return -1;
    }

public:
    bool addCourse(const Course& c) {
        if (c.id.empty() || c.name.empty() || c.credits < 0 || indexOf(c.id) >= 0) return false;
        courses.push_back(c);
        edges.emplace_back();
        return true;
    }

    bool updateCourse(const std::string& id, const std::string& name, int credits) {
        int i = indexOf(id);
        if (i < 0 || name.empty() || credits < 0) return false;
        courses[i].name = name;
        courses[i].credits = credits;
        return true;
    }

    bool addPrerequisite(const std::string& before, const std::string& after) {
        int u = indexOf(before), v = indexOf(after);
        if (u < 0 || v < 0 || u == v) return false;
        if (std::find(edges[u].begin(), edges[u].end(), v) == edges[u].end()) edges[u].push_back(v);
        return true;
    }

    bool removePrerequisite(const std::string& before, const std::string& after) {
        int u = indexOf(before), v = indexOf(after);
        if (u < 0 || v < 0) return false;
        auto& out = edges[u];
        auto it = std::find(out.begin(), out.end(), v);
        if (it == out.end()) return false;
        out.erase(it);
        return true;
    }

    bool removeCourse(const std::string& id) {
        int x = indexOf(id);
        if (x < 0) return false;
        courses.erase(courses.begin() + x);
        edges.erase(edges.begin() + x);
        for (auto& out : edges) {
            out.erase(std::remove(out.begin(), out.end(), x), out.end());
            for (int& v : out) if (v > x) --v;
        }
        return true;
    }

    void list() const {
        std::cout << "课程（编号 | 名称 | 学分）\n";
        for (const auto& c : courses) std::cout << c.id << " | " << c.name << " | " << c.credits << '\n';
        std::cout << "先修约束（先修课 -> 后续课）\n";
        for (int u = 0; u < static_cast<int>(courses.size()); ++u)
            for (int v : edges[u]) std::cout << courses[u].id << " -> " << courses[v].id << '\n';
    }

    // Kahn's algorithm. If not all vertices are removed, residual vertices
    // are in or depend on a cycle; report an actual directed cycle by DFS.
    bool topologicalOrder(std::vector<int>& order, std::vector<int>& cycle) const {
        order.clear();
        cycle.clear();
        std::vector<int> indegree(courses.size(), 0);
        for (const auto& out : edges) for (int v : out) ++indegree[v];
        std::queue<int> q;
        for (int i = 0; i < static_cast<int>(courses.size()); ++i) if (!indegree[i]) q.push(i);
        while (!q.empty()) {
            int u = q.front(); q.pop(); order.push_back(u);
            for (int v : edges[u]) if (--indegree[v] == 0) q.push(v);
        }
        if (order.size() == courses.size()) return true;
        std::vector<int> color(courses.size(), 0), parent(courses.size(), -1);
        auto dfs = [&](auto&& self, int u) -> bool {
            color[u] = 1;
            for (int v : edges[u]) {
                if (indegree[v] == 0) continue; // ignore vertices already removed by Kahn
                if (color[v] == 0) { parent[v] = u; if (self(self, v)) return true; }
                else if (color[v] == 1) {
                    cycle.push_back(v);
                    for (int x = u; x != v; x = parent[x]) cycle.push_back(x);
                    cycle.push_back(v);
                    std::reverse(cycle.begin(), cycle.end());
                    return true;
                }
            }
            color[u] = 2;
            return false;
        };
        for (int i = 0; i < static_cast<int>(courses.size()) && cycle.empty(); ++i)
            if (indegree[i] > 0 && color[i] == 0) dfs(dfs, i);
        return false;
    }

    void showOrder() const {
        std::vector<int> order, cycle;
        if (!topologicalOrder(order, cycle)) {
            std::cout << "检测到先修环，无法生成合法修读顺序。冲突环：";
            for (size_t i = 0; i < cycle.size(); ++i)
                std::cout << (i ? " -> " : "") << courses[cycle[i]].id << '(' << courses[cycle[i]].name << ')';
            std::cout << '\n';
            return;
        }
        std::cout << "合法修读顺序：";
        for (int i : order) std::cout << courses[i].id << '(' << courses[i].name << ") ";
        std::cout << '\n';
    }

    // Each semester is one Kahn layer: all currently available courses.
    // A layer never contains a direct prerequisite conflict.
    void showSemesters(int creditLimit) const {
        std::vector<int> indegree(courses.size(), 0);
        for (const auto& out : edges) for (int v : out) ++indegree[v];
        std::vector<int> ready;
        for (int i = 0; i < static_cast<int>(courses.size()); ++i) if (!indegree[i]) ready.push_back(i);
        int term = 1, done = 0;
        while (!ready.empty()) {
            std::vector<int> layer;
            std::vector<int> deferred;
            int termCredits = 0;
            for (int u : ready) {
                if (creditLimit == 0 || termCredits + courses[u].credits <= creditLimit) {
                    layer.push_back(u);
                    termCredits += courses[u].credits;
                } else deferred.push_back(u);
            }
            if (layer.empty()) {
                std::cout << "学分上限为 " << creditLimit << "，以下课程单门学分已超过上限，无法安排：";
                for (int u : deferred) std::cout << courses[u].id << '(' << courses[u].credits << " 学分) ";
                std::cout << "\n请提高上限或修改课程学分。\n";
                return;
            }
            done += static_cast<int>(layer.size());
            std::cout << "第 " << term++ << " 学期（" << termCredits << " 学分）：";
            for (int u : layer) std::cout << courses[u].id << '(' << courses[u].name << ") ";
            std::cout << '\n';
            std::vector<int> next;
            for (int u : layer) for (int v : edges[u]) if (--indegree[v] == 0) next.push_back(v);
            deferred.insert(deferred.end(), next.begin(), next.end());
            ready = std::move(deferred);
        }
        if (done != static_cast<int>(courses.size())) {
            std::cout << "存在先修环，以下课程无法排入计划：";
            for (int i = 0; i < static_cast<int>(courses.size()); ++i) if (indegree[i] > 0) std::cout << courses[i].id << ' ';
            std::cout << '\n';
        }
    }

    bool load(const std::string& path, std::string* error = nullptr) {
        auto fail = [error](std::size_t line, const std::string& reason) {
            if (error) {
                *error = line == 0 ? reason : "第 " + std::to_string(line) + " 行：" + reason;
            }
            return false;
        };

        std::ifstream in(path, std::ios::binary);
        if (!in) return fail(0, "无法打开文件");

        CoursePlan temp;
        struct PendingEdge {
            std::string before;
            std::string after;
            std::size_t line;
        };
        std::vector<PendingEdge> pendingEdges;
        std::string line;
        std::size_t lineNo = 0;

        // Edges are deferred so a file may list them before its course records.
        while (std::getline(in, line)) {
            ++lineNo;
            if (lineNo == 1 && line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3); // Accept the UTF-8 BOM written by the web export.
            }

            const auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#') continue;

            std::istringstream ss(line.substr(first));
            std::string type, a, b, extra;
            if (!(ss >> type)) continue;

            if (type == "COURSE") {
                int credits = 0;
                if (!(ss >> a >> b >> credits) || (ss >> extra))
                    return fail(lineNo, "课程格式应为 COURSE 编号 名称 学分");
                if (!temp.addCourse({a, b, credits}))
                    return fail(lineNo, "课程编号重复，或课程字段无效");
            } else if (type == "EDGE") {
                if (!(ss >> a >> b) || (ss >> extra))
                    return fail(lineNo, "先修关系格式应为 EDGE 先修课编号 后续课编号");
                pendingEdges.push_back({a, b, lineNo});
            } else {
                return fail(lineNo, "无法识别记录类型：" + type);
            }
        }
        if (in.bad()) return fail(lineNo, "读取文件时发生错误");

        for (const auto& edge : pendingEdges) {
            if (!temp.addPrerequisite(edge.before, edge.after))
                return fail(edge.line, "先修关系引用了不存在的课程，或课程不能依赖自身");
        }

        *this = std::move(temp);
        if (error) error->clear();
        return true;
    }

    bool save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;
        out << "# Course plan format: COURSE id name credits; EDGE prerequisite_id course_id\n";
        for (const auto& c : courses) out << "COURSE " << c.id << ' ' << c.name << ' ' << c.credits << '\n';
        for (int u = 0; u < static_cast<int>(courses.size()); ++u)
            for (int v : edges[u]) out << "EDGE " << courses[u].id << ' ' << courses[v].id << '\n';
        return static_cast<bool>(out);
    }
};

int main(int argc, char** argv) {
#ifdef _WIN32
    // The source and console messages are UTF-8. Set both code pages so they
    // display correctly in Windows Terminal and modern PowerShell consoles.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    CoursePlan plan;
    std::string file = argc > 1 ? argv[1] : "data/chain.txt";
    std::string loadError;
    if (!plan.load(file, &loadError))
        std::cout << "未能读取 " << file << "：" << loadError << "。从空课程表开始。\n";
    for (;;) {
        std::cout << "\n=== 课程先修与教学计划系统 ===\n"
                  << "1. 显示课程与约束  2. 拓扑排序  3. 学期计划\n"
                  << "4. 增加课程        5. 增加先修约束  6. 删除先修约束\n"
                  << "7. 删除课程        8. 保存到文件    9. 修改课程\n0. 退出\n选择：";
        int choice; if (!(std::cin >> choice)) break;
        std::string a, b, name; int credits;
        switch (choice) {
            case 1: plan.list(); break;
            case 2: plan.showOrder(); break;
            case 3: { int limit; std::cout << "输入每学期学分上限（0 表示不限）："; std::cin >> limit;
                    if (limit < 0) std::cout << "学分上限不能为负数\n";
                    else plan.showSemesters(limit);
                    break; }
            case 4: std::cout << "输入 编号 名称 学分："; std::cin >> a >> name >> credits;
                    std::cout << (plan.addCourse({a, name, credits}) ? "课程已添加\n" : "添加失败：编号重复或输入无效\n"); break;
            case 5: std::cout << "输入先修课编号 后续课编号："; std::cin >> a >> b;
                    std::cout << (plan.addPrerequisite(a, b) ? "约束已添加\n" : "添加失败：课程不存在或不能自我依赖\n"); break;
            case 6: std::cout << "输入先修课编号 后续课编号："; std::cin >> a >> b;
                    std::cout << (plan.removePrerequisite(a, b) ? "约束已删除\n" : "未找到该约束\n"); break;
            case 7: std::cout << "输入课程编号："; std::cin >> a;
                    std::cout << (plan.removeCourse(a) ? "课程及关联约束已删除\n" : "未找到该课程\n"); break;
            case 8: std::cout << "保存路径："; std::cin >> a;
                    std::cout << (plan.save(a) ? "保存成功\n" : "保存失败\n"); break;
            case 9: std::cout << "输入课程编号、新名称、新学分："; std::cin >> a >> name >> credits;
                    std::cout << (plan.updateCourse(a, name, credits) ? "课程信息已更新\n" : "修改失败：课程不存在或输入无效\n"); break;
            case 0: return 0;
            default: std::cout << "无效选项\n";
        }
    }
}
