#include "interpreter.h"

#include <climits>
#include <unordered_map>

IRInterpreter::IRInterpreter(int max_instructions) : max_instructions_(max_instructions) {}

ExecutionResult IRInterpreter::execute(const IRProgram& program) {
    ExecutionResult res;
    std::unordered_map<std::string, int32_t> env;
    std::unordered_map<std::string, int> label_map;

    // Index all labels
    for (size_t i = 0; i < program.size(); ++i) {
        if (!program[i].eliminated && program[i].opcode == IROpcode::LABEL) {
            label_map[program[i].label] = static_cast<int>(i);
        }
    }

    auto resolve = [&](const IRValue& val) -> int32_t {
        if (val.is_const) return val.const_val;
        auto it = env.find(val.var_name);
        if (it != env.end()) return it->second;
        return 0; // default for uninitialized
    };

    int pc = 0;
    int instructions_executed = 0;

    while (pc < static_cast<int>(program.size())) {
        if (++instructions_executed > max_instructions_) {
            res.success = false;
            res.error_message = "Runtime limit exceeded (possible infinite loop)";
            break;
        }

        const auto& instr = program[pc];
        if (instr.eliminated) {
            pc++;
            continue;
        }

        switch (instr.opcode) {
            case IROpcode::CONST:
                env[instr.dest] = instr.arg1.const_val;
                pc++;
                break;
            case IROpcode::COPY:
                env[instr.dest] = resolve(instr.arg1);
                pc++;
                break;
            case IROpcode::ADD: {
                uint32_t a = static_cast<uint32_t>(resolve(instr.arg1));
                uint32_t b = static_cast<uint32_t>(resolve(instr.arg2));
                env[instr.dest] = static_cast<int32_t>(a + b);
                pc++;
                break;
            }
            case IROpcode::SUB: {
                uint32_t a = static_cast<uint32_t>(resolve(instr.arg1));
                uint32_t b = static_cast<uint32_t>(resolve(instr.arg2));
                env[instr.dest] = static_cast<int32_t>(a - b);
                pc++;
                break;
            }
            case IROpcode::MUL: {
                uint32_t a = static_cast<uint32_t>(resolve(instr.arg1));
                uint32_t b = static_cast<uint32_t>(resolve(instr.arg2));
                env[instr.dest] = static_cast<int32_t>(a * b);
                pc++;
                break;
            }
            case IROpcode::DIV: {
                int32_t b = resolve(instr.arg2);
                if (b == 0) {
                    res.success = false;
                    res.error_message = "Runtime error: division by zero";
                    return res;
                }
                int32_t a = resolve(instr.arg1);
                if (a == INT32_MIN && b == -1) {
                    env[instr.dest] = INT32_MIN;
                } else {
                    env[instr.dest] = a / b;
                }
                pc++;
                break;
            }
            case IROpcode::MOD: {
                int32_t b = resolve(instr.arg2);
                if (b == 0) {
                    res.success = false;
                    res.error_message = "Runtime error: modulo by zero";
                    return res;
                }
                int32_t a = resolve(instr.arg1);
                if (a == INT32_MIN && b == -1) {
                    env[instr.dest] = 0;
                } else {
                    env[instr.dest] = a % b;
                }
                pc++;
                break;
            }
            case IROpcode::NEG: {
                uint32_t a = static_cast<uint32_t>(resolve(instr.arg1));
                env[instr.dest] = static_cast<int32_t>(~a + 1);
                pc++;
                break;
            }
            case IROpcode::CMP_EQ: {
                env[instr.dest] = (resolve(instr.arg1) == resolve(instr.arg2)) ? 1 : 0;
                pc++;
                break;
            }
            case IROpcode::CMP_NEQ: {
                env[instr.dest] = (resolve(instr.arg1) != resolve(instr.arg2)) ? 1 : 0;
                pc++;
                break;
            }
            case IROpcode::CMP_LT: {
                env[instr.dest] = (resolve(instr.arg1) < resolve(instr.arg2)) ? 1 : 0;
                pc++;
                break;
            }
            case IROpcode::CMP_GT: {
                env[instr.dest] = (resolve(instr.arg1) > resolve(instr.arg2)) ? 1 : 0;
                pc++;
                break;
            }
            case IROpcode::PRINT: {
                int32_t val = resolve(instr.arg1);
                res.output.push_back(std::to_string(val));
                pc++;
                break;
            }
            case IROpcode::LABEL:
                pc++;
                break;
            case IROpcode::JUMP: {
                auto it = label_map.find(instr.label);
                if (it != label_map.end()) {
                    pc = it->second;
                } else {
                    res.success = false;
                    res.error_message = "Runtime error: undefined label '" + instr.label + "'";
                    return res;
                }
                break;
            }
            case IROpcode::JUMP_IF_ZERO: {
                if (resolve(instr.arg1) == 0) {
                    auto it = label_map.find(instr.label);
                    if (it != label_map.end()) {
                        pc = it->second;
                    } else {
                        res.success = false;
                        res.error_message = "Runtime error: undefined label '" + instr.label + "'";
                        return res;
                    }
                } else {
                    pc++;
                }
                break;
            }
            case IROpcode::NOP:
                pc++;
                break;
        }
    }

    res.instructions_executed = instructions_executed;
    return res;
}
