#include "json_output.h"

#include <sstream>

std::string JSONOutput::escapeString(const std::string& str) {
    std::ostringstream ss;
    for (char c : str) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b";  break;
            case '\f': ss << "\\f";  break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control characters
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    ss << buf;
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

static std::string serializeIRList(const IRProgram& ir) {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < ir.size(); ++i) {
        const auto& instr = ir[i];
        ss << "      {\n"
           << "        \"id\": " << instr.id << ",\n"
           << "        \"opcode\": \"" << opcodeToString(instr.opcode) << "\",\n"
           << "        \"dest\": \"" << JSONOutput::escapeString(instr.dest) << "\",\n"
           << "        \"arg1\": \"" << JSONOutput::escapeString(instr.arg1.toString()) << "\",\n"
           << "        \"arg2\": \"" << JSONOutput::escapeString(instr.arg2.toString()) << "\",\n"
           << "        \"label\": \"" << JSONOutput::escapeString(instr.label) << "\",\n"
           << "        \"eliminated\": " << (instr.eliminated ? "true" : "false") << ",\n"
           << "        \"source_line\": " << instr.source_line << ",\n"
           << "        \"text\": \"" << JSONOutput::escapeString(instr.toString()) << "\"\n"
           << "      }" << (i + 1 < ir.size() ? "," : "") << "\n";
    }
    ss << "    ]";
    return ss.str();
}

std::string JSONOutput::serialize(const CompilationReport& report, const Program* /* ast */) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"success\": " << (report.success ? "true" : "false") << ",\n";
    ss << "  \"source\": \"" << escapeString(report.source) << "\",\n";

    // Tokens
    ss << "  \"tokens\": [\n";
    for (size_t i = 0; i < report.tokens.size(); ++i) {
        const auto& tok = report.tokens[i];
        ss << "    {\"type\": \"" << tokenTypeToString(tok.type) << "\", "
           << "\"value\": \"" << escapeString(tok.value) << "\", "
           << "\"line\": " << tok.line << ", "
           << "\"column\": " << tok.column << "}"
           << (i + 1 < report.tokens.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Initial IR
    ss << "  \"initial_ir\": " << serializeIRList(report.initial_ir) << ",\n";

    // Passes
    ss << "  \"passes\": [\n";
    for (size_t i = 0; i < report.snapshots.size(); ++i) {
        const auto& snap = report.snapshots[i];
        ss << "    {\n"
           << "      \"name\": \"" << escapeString(snap.pass_name) << "\",\n"
           << "      \"iteration\": " << snap.iteration << ",\n"
           << "      \"changed\": " << (snap.changed ? "true" : "false") << ",\n"
           << "      \"explanations\": [\n";
        for (size_t j = 0; j < snap.explanations.size(); ++j) {
            ss << "        \"" << escapeString(snap.explanations[j]) << "\""
               << (j + 1 < snap.explanations.size() ? "," : "") << "\n";
        }
        ss << "      ],\n"
           << "      \"ir_after\": " << serializeIRList(snap.ir_after) << "\n"
           << "    }" << (i + 1 < report.snapshots.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Final IR
    ss << "  \"final_ir\": " << serializeIRList(report.final_ir) << ",\n";

    // Execution Before
    ss << "  \"execution_before\": {\n"
       << "    \"success\": " << (report.exec_before.success ? "true" : "false") << ",\n"
       << "    \"error\": \"" << escapeString(report.exec_before.error_message) << "\",\n"
       << "    \"instructions_executed\": " << report.exec_before.instructions_executed << ",\n"
       << "    \"output\": [";
    for (size_t i = 0; i < report.exec_before.output.size(); ++i) {
        ss << "\"" << escapeString(report.exec_before.output[i]) << "\""
           << (i + 1 < report.exec_before.output.size() ? ", " : "");
    }
    ss << "]\n  },\n";

    // Execution After
    ss << "  \"execution_after\": {\n"
       << "    \"success\": " << (report.exec_after.success ? "true" : "false") << ",\n"
       << "    \"error\": \"" << escapeString(report.exec_after.error_message) << "\",\n"
       << "    \"instructions_executed\": " << report.exec_after.instructions_executed << ",\n"
       << "    \"output\": [";
    for (size_t i = 0; i < report.exec_after.output.size(); ++i) {
        ss << "\"" << escapeString(report.exec_after.output[i]) << "\""
           << (i + 1 < report.exec_after.output.size() ? ", " : "");
    }
    ss << "]\n  },\n";

    ss << "  \"results_match\": " << (report.results_match ? "true" : "false") << ",\n";

    // Errors
    ss << "  \"errors\": [\n";
    for (size_t i = 0; i < report.errors.size(); ++i) {
        const auto& err = report.errors[i];
        ss << "    {\"message\": \"" << escapeString(err.message) << "\", "
           << "\"line\": " << err.line << ", "
           << "\"column\": " << err.column << ", "
           << "\"phase\": \"" << escapeString(err.phase) << "\"}"
           << (i + 1 < report.errors.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}
