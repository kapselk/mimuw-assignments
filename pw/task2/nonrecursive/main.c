#include <stddef.h>

#include "common/io.h"
#include "common/sumset.h"

#define MAX_STACK_SIZE 3000

typedef struct {
    Sumset a;
    Sumset* b;
    bool stage;
} StackFrame;

static InputData input_data;

static Solution best_solution;

static void solve(const Sumset* a0, const Sumset* b0) {
    StackFrame stack[MAX_STACK_SIZE];
    StackFrame* top = &stack[1];
    StackFrame* beg = &stack[0];
    top->a = *a0;
    top->b = (Sumset*)b0;
    top->stage = false;
    while (beg != top) {
        if (top->stage) {
            top--;
        } else {
            StackFrame* frame = top;
            Sumset* a = &frame->a;
            Sumset* b = frame->b;
            if (a->sum > b->sum) {
                Sumset* tmp = a;
                a = b;
                b = tmp;
            }
            if (is_sumset_intersection_trivial(a, b)) {
                bool child = false;
                for (size_t i = input_data.d; i >= a->last; i--) {
                    // reverse order
                    if (!does_sumset_contain(b, i)) {
                        top++;
                        Sumset* a_with_i = &top->a;
                        sumset_add(a_with_i, a, i);
                        top->b = b;
                        top->stage = false;
                        child = true;
                    }
                }
                if (child) {
                    frame->stage = true;
                    continue;
                }
            } else if ((a->sum == b->sum) && (get_sumset_intersection_size(a, b) == 2)) {
                if (b->sum > best_solution.sum) {
                    solution_build(&best_solution, &input_data, a, b);
                }
            }
            top--;
        }
    }
}

int main() {
    input_data_read(&input_data);
    // input_data_init(&input_data, 8, 10, (int[]){0}, (int[]){1, 0});

    solution_init(&best_solution);
    solve(&input_data.a_start, &input_data.b_start);
    solution_print(&best_solution);
    return 0;
}
