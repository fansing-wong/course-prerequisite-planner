#pragma once

#include <string>
#include <vector>

namespace courseplan {

struct Course {
    std::string id;
    std::string name;
    int credits = 0;
};

struct TopologyResult {
    std::vector<int> order;
    std::vector<int> cycle;     // Closed path: the first index is repeated at the end.
    std::vector<int> unresolved; // Cycle vertices and courses downstream from a cycle.

    bool acyclic() const { return unresolved.empty(); }
};

struct SemesterPlan {
    std::vector<std::vector<int>> semesters;
    std::vector<int> overLimit; // Currently ready courses that cannot fit even alone.
    std::vector<int> unresolved; // Courses left because they are in or depend on a cycle.
    std::vector<int> cycle;
    bool validCreditLimit = true;

    bool complete() const { return validCreditLimit && overLimit.empty() && unresolved.empty(); }
};

// Course prerequisite planner: an AOV network stored as an adjacency list.
// Edge u -> v means course u must be completed before course v.
class CoursePlan {
public:
    const std::vector<Course>& courses() const { return courses_; }
    const std::vector<std::vector<int>>& edges() const { return edges_; }

    bool addCourse(const Course& course);
    bool updateCourse(const std::string& id, const std::string& name, int credits);
    bool addPrerequisite(const std::string& before, const std::string& after);
    bool removePrerequisite(const std::string& before, const std::string& after);
    bool removeCourse(const std::string& id);

    TopologyResult topologicalOrder() const;
    SemesterPlan buildSemesters(int creditLimit) const;

    // A failed load leaves the current course plan unchanged.
    bool load(const std::string& path, std::string* error = nullptr);
    bool save(const std::string& path) const;

private:
    std::vector<Course> courses_;
    std::vector<std::vector<int>> edges_;

    int indexOf(const std::string& id) const;
};

} // namespace courseplan
