#include "htn.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"

TotalOrderForwardDecomposition::TotalOrderForwardDecomposition(const PlanningProblem &planningProblem) :
		planning_problem(planningProblem) {
}

TotalOrderForwardDecomposition::~TotalOrderForwardDecomposition() {}

TotalOrderForwardDecomposition::Plan TotalOrderForwardDecomposition::try_to_plan() {
	std::vector<Task> tasks;
	Plan solutionPlan;
	tasks.push_back(planning_problem.get_top_level_task());

	print_verbose(vformat("Try To Plan for: %s\n", tasks.back().task_name.c_str()));

	return seek_plan(tasks, planning_problem.get_initial_state(), solutionPlan);
}

TotalOrderForwardDecomposition::Plan TotalOrderForwardDecomposition::seek_plan(const std::vector<Task> &p_tasks, const State &p_current_state, Plan &p_current_plan) {
	if (p_tasks.empty()) {
		print_verbose("SeekPlan: No more tasks, returning current plan.\n");
		if (!p_current_plan.empty()) {
			print_verbose("TFD found solution plan.");
			for (const OperatorWithParams &operatorWithParams : p_current_plan) {
				print_verbose(operatorWithParams.task.task_name.c_str());
			}
		}
		return p_current_plan;
	}

	if (planning_problem.task_is_operator(p_tasks.back().task_name)) {
		print_verbose("SeekPlan: Task is operator type.\n");
		return search_operators(p_tasks, p_current_state, p_current_plan);
	}

	if (planning_problem.task_is_method(p_tasks.back().task_name)) {
		print_verbose("SeekPlan: Task is method type.\n");
		return search_methods(p_tasks, p_current_state, p_current_plan);
	}

	return {};
}

TotalOrderForwardDecomposition::Plan TotalOrderForwardDecomposition::search_methods(const std::vector<Task> &p_tasks, const State &p_current_state, Plan &p_current_plan) {
	print_verbose(vformat("SearchMethods for %s", p_tasks.back().task_name.c_str()));
	RelevantMethods relevantMethods = planning_problem.get_methods_for_task(p_tasks.back(), p_current_state);

	if (!relevantMethods.empty()) {
		print_verbose(vformat("SearchMethods: %d relevant methods found.", relevantMethods.size()));

		for (const MethodWithParams &relevantMethod : relevantMethods) {
			std::optional<std::vector<Task>> subTasks = relevantMethod.func(p_current_state, relevantMethod.task.parameters);
			if (subTasks && !subTasks.value().empty()) {
				std::vector<Task> new_tasks(p_tasks);
				new_tasks.pop_back();
				new_tasks.insert(new_tasks.end(), subTasks.value().begin(), subTasks.value().end());

				std::vector<OperatorWithParams> solution = seek_plan(new_tasks, p_current_state, p_current_plan);
				if (!solution.empty()) {
					return solution;
				}
			}
		}
	} else {
		print_verbose("SearchMethods: No relevant methods found.");
	}

	print_verbose("SearchMethods: Failed to plan");
	return {};
}

TotalOrderForwardDecomposition::Plan TotalOrderForwardDecomposition::search_operators(const std::vector<Task> &p_tasks, const State &p_current_state, Plan &p_current_plan) {
	print_verbose(vformat("SearchOperators for %s", p_tasks.back().task_name.c_str()));
	ApplicableOperators applicableOperators = planning_problem.get_operators_for_task(p_tasks.back(), p_current_state);

	if (!applicableOperators.empty()) {
		for (const OperatorWithParams &chosenOperator : applicableOperators) {
			const std::optional<State> new_state = chosenOperator.func(p_current_state, chosenOperator.task.parameters);

			if (new_state) {
				std::vector<Task> new_tasks(p_tasks);
				new_tasks.pop_back();
				p_current_plan.push_back(chosenOperator);

				std::vector<OperatorWithParams> solution = seek_plan(new_tasks, new_state.value(), p_current_plan);
				if (!solution.empty()) {
					return solution;
				}
			}
		}
	} else {
		print_verbose("SearchOperators: No applicable operator found.");
	}

	return {};
}