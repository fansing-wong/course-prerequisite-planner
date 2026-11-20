#include "course_plan_core.hpp"

#include <iostream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

using courseplan::CoursePlan;

void showCourses(const CoursePlan& plan) {
    std::cout << "课程（编号 | 名称 | 学分）\n";
    for (const auto& course : plan.courses())
        std::cout << course.id << " | " << course.name << " | " << course.credits << '\n';

    std::cout << "先修约束（先修课 -> 后续课）\n";
    const auto& courses = plan.courses();
    const auto& edges = plan.edges();
    for (int from = 0; from < static_cast<int>(courses.size()); ++from) {
        for (int to : edges[from]) std::cout << courses[from].id << " -> " << courses[to].id << '\n';
    }
}

void showOrder(const CoursePlan& plan) {
    const auto result = plan.topologicalOrder();
    const auto& courses = plan.courses();
    if (!result.acyclic()) {
        std::cout << "检测到先修环，无法生成合法修读顺序。冲突环：";
        for (std::size_t i = 0; i < result.cycle.size(); ++i) {
            const auto& course = courses[result.cycle[i]];
            std::cout << (i ? " -> " : "") << course.id << '(' << course.name << ')';
        }
        std::cout << '\n';
        return;
    }

    std::cout << "合法修读顺序：";
    for (int index : result.order)
        std::cout << courses[index].id << '(' << courses[index].name << ") ";
    std::cout << '\n';
}

void showSemesters(const CoursePlan& plan, int creditLimit) {
    const auto result = plan.buildSemesters(creditLimit);
    if (!result.validCreditLimit) {
        std::cout << "学分上限不能为负数。\n";
        return;
    }

    const auto& courses = plan.courses();
    for (std::size_t term = 0; term < result.semesters.size(); ++term) {
        const auto& semester = result.semesters[term];
        int credits = 0;
        for (int index : semester) credits += courses[index].credits;

        std::cout << "第 " << term + 1 << " 学期（" << credits << " 学分）：";
        for (int index : semester)
            std::cout << courses[index].id << '(' << courses[index].name << ") ";
        std::cout << '\n';
    }

    if (!result.overLimit.empty()) {
        std::cout << "学分上限为 " << creditLimit << "，以下可修课程单门学分已超过上限：";
        for (int index : result.overLimit)
            std::cout << courses[index].id << '(' << courses[index].credits << " 学分) ";
        std::cout << "\n请提高上限或修改课程学分。\n";
    }

    if (!result.unresolved.empty()) {
        std::cout << "存在先修环，以下课程在环中或受环影响，无法排入计划：";
        for (int index : result.unresolved) std::cout << courses[index].id << ' ';
        std::cout << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    // Source and console text are UTF-8 on Windows.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    CoursePlan plan;
    const std::string file = argc > 1 ? argv[1] : "data/chain.txt";
    std::string loadError;
    if (!plan.load(file, &loadError))
        std::cout << "未能读取 " << file << "：" << loadError << "。从空课程表开始。\n";

    for (;;) {
        std::cout << "\n=== 课程先修与教学计划系统 ===\n"
                  << "1. 显示课程与约束  2. 拓扑排序  3. 学期计划\n"
                  << "4. 增加课程        5. 增加先修约束  6. 删除先修约束\n"
                  << "7. 删除课程        8. 保存到文件    9. 修改课程\n0. 退出\n选择：";
        int choice;
        if (!(std::cin >> choice)) break;

        std::string a, b, name;
        int credits = 0;
        switch (choice) {
            case 1:
                showCourses(plan);
                break;
            case 2:
                showOrder(plan);
                break;
            case 3: {
                int limit = 0;
                std::cout << "输入每学期学分上限（0 表示不限）：";
                if (!(std::cin >> limit)) {
                    std::cout << "请输入整数学分上限。\n";
                    return 0;
                }
                showSemesters(plan, limit);
                break;
            }
            case 4:
                std::cout << "输入 编号 名称 学分：";
                if (!(std::cin >> a >> name >> credits)) return 0;
                std::cout << (plan.addCourse({a, name, credits})
                                  ? "课程已添加\n"
                                  : "添加失败：编号重复或输入无效\n");
                break;
            case 5:
                std::cout << "输入先修课编号 后续课编号：";
                if (!(std::cin >> a >> b)) return 0;
                std::cout << (plan.addPrerequisite(a, b)
                                  ? "约束已添加\n"
                                  : "添加失败：课程不存在或不能自我依赖\n");
                break;
            case 6:
                std::cout << "输入先修课编号 后续课编号：";
                if (!(std::cin >> a >> b)) return 0;
                std::cout << (plan.removePrerequisite(a, b) ? "约束已删除\n" : "未找到该约束\n");
                break;
            case 7:
                std::cout << "输入课程编号：";
                if (!(std::cin >> a)) return 0;
                std::cout << (plan.removeCourse(a) ? "课程及关联约束已删除\n" : "未找到该课程\n");
                break;
            case 8:
                std::cout << "保存路径：";
                if (!(std::cin >> a)) return 0;
                std::cout << (plan.save(a) ? "保存成功\n" : "保存失败\n");
                break;
            case 9:
                std::cout << "输入课程编号、新名称、新学分：";
                if (!(std::cin >> a >> name >> credits)) return 0;
                std::cout << (plan.updateCourse(a, name, credits)
                                  ? "课程信息已更新\n"
                                  : "修改失败：课程不存在或输入无效\n");
                break;
            case 0:
                return 0;
            default:
                std::cout << "无效选项\n";
        }
    }
    return 0;
}
