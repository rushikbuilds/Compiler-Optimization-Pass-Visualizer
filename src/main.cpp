#include "constant_folding.h"
#include "constant_propagation.h"
#include "dead_code_elimination.h"
#include "interpreter.h"
#include "ir_generator.h"
#include "json_output.h"
#include "lexer.h"
#include "parser.h"

#include <fstream>
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    bool enable_cf = true;
    bool enable_cp = true;
    bool enable_dce = true;
    int max_iterations = 10;
    std::string input_file;
    CompilationReport report;

    auto argumentError = [&](const std::string& message) {
        report.success = false;
        report.errors.push_back(CompileError{message, 0, 0, "cli"});
        std::cout << JSONOutput::serialize(report);
        return 2;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-cf") enable_cf = false;
        else if (arg == "--no-cp") enable_cp = false;
        else if (arg == "--no-dce") enable_dce = false;
        else if (arg == "--cf") enable_cf = true;
        else if (arg == "--cp") enable_cp = true;
        else if (arg == "--dce") enable_dce = true;
        else if (arg == "--max-iterations") {
            if (i + 1 >= argc) {
                return argumentError("Missing value for --max-iterations");
            }
            try {
                const long long parsed = std::stoll(argv[++i]);
                if (parsed < 1 || parsed > 1000) {
                    return argumentError("--max-iterations must be between 1 and 1000");
                }
                max_iterations = static_cast<int>(parsed);
            } catch (const std::exception&) {
                return argumentError("Invalid integer value for --max-iterations");
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: compiler [options] [file.mini]\n"
                      << "Options:\n"
                      << "  --[no-]cf            Enable/disable Constant Folding (default: enabled)\n"
                      << "  --[no-]cp            Enable/disable Constant Propagation (default: enabled)\n"
                      << "  --[no-]dce           Enable/disable Dead Code Elimination (default: enabled)\n"
                      << "  --max-iterations N   Max fixed-point iterations (default: 10)\n";
            return 0;
        } else if (!arg.empty() && arg[0] != '-') {
            if (!input_file.empty()) {
                return argumentError("Only one input file may be specified");
            }
            input_file = arg;
        } else {
            return argumentError("Unknown option '" + arg + "'");
        }
    }

    std::string source;
    if (!input_file.empty()) {
        std::ifstream file(input_file);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << input_file << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();
    }

    report.source = source;

    // 1. Lexing
    Lexer lexer(source);
    report.tokens = lexer.tokenize();
    if (lexer.hasErrors()) {
        report.success = false;
        report.errors.insert(report.errors.end(), lexer.getErrors().begin(), lexer.getErrors().end());
        std::cout << JSONOutput::serialize(report);
        return 0;
    }

    // 2. Parsing
    Parser parser(report.tokens);
    Program ast = parser.parse();
    if (parser.hasErrors()) {
        report.success = false;
        report.errors.insert(report.errors.end(), parser.getErrors().begin(), parser.getErrors().end());
        std::cout << JSONOutput::serialize(report, &ast);
        return 0;
    }

    // 3. IR Generation
    IRGenerator ir_gen;
    report.initial_ir = ir_gen.generate(ast);
    if (ir_gen.hasErrors()) {
        report.success = false;
        report.errors.insert(report.errors.end(), ir_gen.getErrors().begin(), ir_gen.getErrors().end());
        std::cout << JSONOutput::serialize(report, &ast);
        return 0;
    }

    // 4. Initial Execution Verification
    IRInterpreter interpreter;
    report.exec_before = interpreter.execute(report.initial_ir);

    // 5. Optimization Pipeline (Fixed-Point Iteration)
    std::vector<std::pair<std::string, std::unique_ptr<OptimizationPass>>> passes;
    if (enable_cf) {
        passes.emplace_back("Constant Folding", std::make_unique<ConstantFoldingPass>());
    }
    if (enable_cp) {
        passes.emplace_back("Constant Propagation", std::make_unique<ConstantPropagationPass>());
    }
    if (enable_dce) {
        passes.emplace_back("Dead Code Elimination", std::make_unique<DeadCodeEliminationPass>());
    }

    IRProgram current_ir = report.initial_ir;

    for (int iter = 1; iter <= max_iterations; ++iter) {
        bool iter_changed = false;

        for (auto& [name, pass] : passes) {
            auto result = pass->run(current_ir);

            PassSnapshot snap;
            snap.pass_name = name;
            snap.iteration = iter;
            snap.changed = result.changed;
            snap.explanations = result.explanations;
            snap.ir_after = current_ir;
            report.snapshots.push_back(snap);

            if (result.changed) {
                iter_changed = true;
            }
        }

        // Fixed point reached: no pass made any change in this round
        if (!iter_changed) {
            break;
        }
    }

    report.final_ir = current_ir;

    // 6. Optimized Execution Verification
    report.exec_after = interpreter.execute(report.final_ir);

    // 7. Verify Equivalence
    report.results_match = (report.exec_before.success == report.exec_after.success) &&
                           (report.exec_before.output == report.exec_after.output);
    if (!report.exec_before.success || !report.exec_after.success) {
        report.success = false;
        if (!report.exec_before.success) {
            report.errors.push_back(CompileError{
                report.exec_before.error_message, 0, 0, "runtime_before"
            });
        }
        if (!report.exec_after.success) {
            report.errors.push_back(CompileError{
                report.exec_after.error_message, 0, 0, "runtime_after"
            });
        }
    }

    std::cout << JSONOutput::serialize(report, &ast);
    return 0;
}
