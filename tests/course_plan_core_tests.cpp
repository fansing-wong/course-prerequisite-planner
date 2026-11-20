#include "course_plan_core.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

using courseplan::Course;
using courseplan::CoursePlan;
using courseplan::SemesterPlan;
using courseplan::TopologyResult;

int checks = 0;
int failures = 0;

#define CHECK(condition)                                                                    \
    do {                                                                                    \
        ++checks;                                                                           \
        if (!(condition)) {                                                                 \
            ++failures;                                                                     \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #condition << '\n'; \
        }                                                                                   \
    } while (false)

class TemporaryFiles {
public:
    TemporaryFiles() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        prefix_ = "course_plan_test_" + std::to_string(stamp);
    }

    ~TemporaryFiles() {
        for (const auto& path : paths_) {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    }

    std::filesystem::path make(const std::string& suffix) {
        auto path = std::filesystem::temp_directory_path() / (prefix_ + suffix);
        paths_.push_back(path);
        return path;
    }

private:
    std::string prefix_;
    std::vector<std::filesystem::path> paths_;
};

int indexOf(const CoursePlan& plan, const std::string& id) {
    for (int i = 0; i < static_cast<int>(plan.courses().size()); ++i) {
        if (plan.courses()[i].id == id) return i;
    }
    return -1;
}

bool hasEdge(const CoursePlan& plan, int from, int to) {
    const auto& outgoing = plan.edges()[from];
    for (int target : outgoing) {
        if (target == to) return true;
    }
    return false;
}

void addCourse(CoursePlan& plan, const std::string& id, int credits = 3) {
    CHECK(plan.addCourse({id, "Course_" + id, credits}));
}

void checkValidTopologicalOrder(const CoursePlan& plan, const TopologyResult& result) {
    CHECK(result.acyclic());
    CHECK(result.order.size() == plan.courses().size());

    std::vector<int> position(plan.courses().size(), -1);
    for (int i = 0; i < static_cast<int>(result.order.size()); ++i) {
        const int course = result.order[i];
        CHECK(course >= 0 && course < static_cast<int>(position.size()));
        if (course >= 0 && course < static_cast<int>(position.size())) position[course] = i;
    }
    for (int from = 0; from < static_cast<int>(plan.edges().size()); ++from) {
        for (int to : plan.edges()[from]) CHECK(position[from] < position[to]);
    }
}

bool isValidClosedCycle(const CoursePlan& plan, const std::vector<int>& cycle) {
    if (cycle.size() < 2 || cycle.front() != cycle.back()) return false;
    for (std::size_t i = 1; i < cycle.size(); ++i) {
        if (!hasEdge(plan, cycle[i - 1], cycle[i])) return false;
    }
    return true;
}

void checkSemesterConstraints(const CoursePlan& plan, const SemesterPlan& schedule, int creditLimit) {
    std::vector<int> termOf(plan.courses().size(), -1);
    for (int term = 0; term < static_cast<int>(schedule.semesters.size()); ++term) {
        int credits = 0;
        for (int course : schedule.semesters[term]) {
            CHECK(course >= 0 && course < static_cast<int>(termOf.size()));
            if (course >= 0 && course < static_cast<int>(termOf.size())) {
                CHECK(termOf[course] == -1);
                termOf[course] = term;
                credits += plan.courses()[course].credits;
            }
        }
        if (creditLimit > 0) CHECK(credits <= creditLimit);
    }

    for (int from = 0; from < static_cast<int>(plan.edges().size()); ++from) {
        for (int to : plan.edges()[from]) {
            if (termOf[from] >= 0 && termOf[to] >= 0) CHECK(termOf[from] < termOf[to]);
        }
    }
}

void testTopologicalSort() {
    CoursePlan empty;
    checkValidTopologicalOrder(empty, empty.topologicalOrder());

    CoursePlan noEdges;
    addCourse(noEdges, "A");
    addCourse(noEdges, "B");
    addCourse(noEdges, "C");
    checkValidTopologicalOrder(noEdges, noEdges.topologicalOrder());

    CoursePlan chain;
    addCourse(chain, "A");
    addCourse(chain, "B");
    addCourse(chain, "C");
    CHECK(chain.addPrerequisite("A", "B"));
    CHECK(chain.addPrerequisite("B", "C"));
    const auto chainResult = chain.topologicalOrder();
    checkValidTopologicalOrder(chain, chainResult);
    CHECK(chainResult.order == std::vector<int>({0, 1, 2}));

    CoursePlan diamond;
    addCourse(diamond, "A");
    addCourse(diamond, "B");
    addCourse(diamond, "C");
    addCourse(diamond, "D");
    CHECK(diamond.addPrerequisite("A", "C"));
    CHECK(diamond.addPrerequisite("B", "C"));
    CHECK(diamond.addPrerequisite("C", "D"));
    // A and B may appear in either order; test the constraints, not one queue order.
    checkValidTopologicalOrder(diamond, diamond.topologicalOrder());
}

void testCycleDetection() {
    CoursePlan plan;
    addCourse(plan, "A");
    addCourse(plan, "B");
    addCourse(plan, "C");
    addCourse(plan, "D"); // Downstream of the cycle, but not itself part of it.
    addCourse(plan, "FREE");
    CHECK(plan.addPrerequisite("A", "B"));
    CHECK(plan.addPrerequisite("B", "C"));
    CHECK(plan.addPrerequisite("C", "A"));
    CHECK(plan.addPrerequisite("C", "D"));

    const auto result = plan.topologicalOrder();
    CHECK(!result.acyclic());
    CHECK(result.order == std::vector<int>({4}));
    CHECK(isValidClosedCycle(plan, result.cycle));
    CHECK(result.unresolved == std::vector<int>({0, 1, 2, 3}));

    const auto schedule = plan.buildSemesters(0);
    CHECK(schedule.semesters.size() == 1);
    CHECK(schedule.semesters[0] == std::vector<int>({4}));
    CHECK(schedule.cycle == result.cycle);
    CHECK(schedule.unresolved == result.unresolved);
}

void testSemesterPlanning() {
    CoursePlan plan;
    addCourse(plan, "A", 5);
    addCourse(plan, "B", 4);
    addCourse(plan, "C", 3);
    addCourse(plan, "D", 3);
    CHECK(plan.addPrerequisite("A", "C"));
    CHECK(plan.addPrerequisite("B", "D"));

    const auto capped = plan.buildSemesters(7);
    CHECK(capped.complete());
    CHECK(capped.semesters.size() == 3);
    checkSemesterConstraints(plan, capped, 7);

    const auto unlimited = plan.buildSemesters(0);
    CHECK(unlimited.complete());
    CHECK(unlimited.semesters.size() == 2);
    checkSemesterConstraints(plan, unlimited, 0);

    CoursePlan tooLarge;
    addCourse(tooLarge, "BIG", 9);
    const auto blocked = tooLarge.buildSemesters(8);
    CHECK(!blocked.complete());
    CHECK(blocked.semesters.empty());
    CHECK(blocked.overLimit == std::vector<int>({0}));

    const auto invalidLimit = tooLarge.buildSemesters(-1);
    CHECK(!invalidLimit.validCreditLimit);
    CHECK(!invalidLimit.complete());
}

void testCourseAndEdgeOperations() {
    CoursePlan plan;
    CHECK(plan.addCourse({"A", "Alpha", 3}));
    CHECK(plan.addCourse({"B", "Beta", 4}));
    CHECK(plan.addCourse({"C", "Gamma", 2}));
    CHECK(!plan.addCourse({"A", "Duplicate", 1}));
    CHECK(!plan.addCourse({"", "No id", 1}));
    CHECK(!plan.addCourse({"BAD", "Negative", -1}));

    CHECK(!plan.addPrerequisite("A", "MISSING"));
    CHECK(!plan.addPrerequisite("A", "A"));
    CHECK(plan.addPrerequisite("A", "B"));
    CHECK(plan.addPrerequisite("A", "B")); // Duplicate insertion is idempotent.
    CHECK(plan.edges()[0].size() == 1);
    CHECK(!plan.removePrerequisite("B", "C"));
    CHECK(plan.removePrerequisite("A", "B"));
    CHECK(!plan.removePrerequisite("A", "B"));

    CHECK(plan.addPrerequisite("A", "B"));
    CHECK(plan.addPrerequisite("B", "C"));
    CHECK(plan.updateCourse("B", "Updated", 5));
    CHECK(!plan.updateCourse("MISSING", "Updated", 5));
    CHECK(plan.courses()[1].name == "Updated");
    CHECK(plan.removeCourse("B"));
    CHECK(!plan.removeCourse("B"));
    CHECK(plan.courses().size() == 2);
    CHECK(plan.courses()[1].id == "C");
    CHECK(plan.edges().size() == 2);
    CHECK(plan.edges()[0].empty());
    CHECK(plan.edges()[1].empty());
    checkValidTopologicalOrder(plan, plan.topologicalOrder());
}

void writeFile(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    CHECK(static_cast<bool>(output));
}

void testFileLoadingAndSaving() {
    TemporaryFiles files;
    const auto importedPath = files.make("_bom_and_edge_order.txt");
    const std::string webExport =
        "\xEF\xBB\xBF# Exported from the browser\n"
        "  EDGE C01 C02\n"
        "COURSE C01 程序设计 4\n"
        "COURSE C02 数据结构 4\n";
    writeFile(importedPath, webExport);

    CoursePlan imported;
    std::string error;
    CHECK(imported.load(importedPath.string(), &error));
    CHECK(error.empty());
    CHECK(imported.courses().size() == 2);
    CHECK(imported.courses()[0].name == "程序设计");
    CHECK(hasEdge(imported, indexOf(imported, "C01"), indexOf(imported, "C02")));
    checkValidTopologicalOrder(imported, imported.topologicalOrder());

    const auto savedPath = files.make("_round_trip.txt");
    CHECK(imported.save(savedPath.string()));
    CoursePlan roundTrip;
    CHECK(roundTrip.load(savedPath.string(), &error));
    CHECK(roundTrip.courses().size() == imported.courses().size());
    CHECK(roundTrip.courses()[0].id == imported.courses()[0].id);
    CHECK(roundTrip.courses()[0].name == imported.courses()[0].name);
    CHECK(roundTrip.courses()[0].credits == imported.courses()[0].credits);
    CHECK(roundTrip.edges() == imported.edges());

    CoursePlan preserved;
    addCourse(preserved, "KEEP");
    const auto badFormatPath = files.make("_bad_format.txt");
    writeFile(badFormatPath, "# valid comment\nCOURSE ONLY_TWO_FIELDS\n");
    CHECK(!preserved.load(badFormatPath.string(), &error));
    CHECK(error.find("第 2 行") != std::string::npos);
    CHECK(preserved.courses().size() == 1);
    CHECK(preserved.courses()[0].id == "KEEP");

    const auto missingCoursePath = files.make("_missing_course.txt");
    writeFile(missingCoursePath, "COURSE A Alpha 3\nEDGE A MISSING\n");
    CHECK(!preserved.load(missingCoursePath.string(), &error));
    CHECK(error.find("第 2 行") != std::string::npos);
    CHECK(preserved.courses().size() == 1);

    const auto extraFieldPath = files.make("_extra_field.txt");
    writeFile(extraFieldPath, "COURSE A Alpha 3 unexpected\n");
    CHECK(!preserved.load(extraFieldPath.string(), &error));
    CHECK(error.find("第 1 行") != std::string::npos);
}

} // namespace

int main() {
    testTopologicalSort();
    testCycleDetection();
    testSemesterPlanning();
    testCourseAndEdgeOperations();
    testFileLoadingAndSaving();

    if (failures != 0) {
        std::cerr << failures << " of " << checks << " checks failed.\n";
        return 1;
    }
    std::cout << "All " << checks << " checks passed.\n";
    return 0;
}
