#include "compiler/parser.h"

#include <charconv>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

namespace torture::compiler {
namespace {

class Parser {
public:
    Parser(std::span<const Token> tokens, Diagnostics& diagnostics)
        : tokens_(tokens), diagnostics_(diagnostics) {}

    std::optional<Program> parseProgram() {
        Program program;
        if (match("require")) {
            if (!expect("ecc", "expected ecc after require") || !expect(";", "expected ; after require ecc")) {
                return std::nullopt;
            }
            program.requireEcc = true;
        }

        while (!atEnd()) {
            if (check("function")) {
                auto function = parseFunction(false);
                if (!function) {
                    return std::nullopt;
                }
                program.functions.push_back(std::move(*function));
                continue;
            }
            if (check("struct")) {
                auto structure = parseStruct();
                if (!structure) {
                    return std::nullopt;
                }
                program.structs.push_back(std::move(*structure));
                continue;
            }
            if (check("class")) {
                auto classDecl = parseClass();
                if (!classDecl) {
                    return std::nullopt;
                }
                program.structs.push_back(std::move(*classDecl));
                continue;
            }
            error(current(), "unexpected-top-level", "expected function, struct, or class declaration");
            return std::nullopt;
        }

        return program;
    }

private:
    bool atEnd() const {
        return current().kind == TokenKind::End;
    }

    const Token& current() const {
        return tokens_[position_];
    }

    const Token& previous() const {
        return tokens_[position_ - 1];
    }

    bool check(std::string_view text) const {
        return current().text == text;
    }

    bool checkKind(TokenKind kind) const {
        return current().kind == kind;
    }

    bool match(std::string_view text) {
        if (!check(text)) {
            return false;
        }
        ++position_;
        return true;
    }

    bool matchKind(TokenKind kind) {
        if (!checkKind(kind)) {
            return false;
        }
        ++position_;
        return true;
    }

    bool expect(std::string_view text, std::string message) {
        if (match(text)) {
            return true;
        }
        error(current(), "expected-token", std::move(message));
        return false;
    }

    std::optional<Token> expectIdentifier(std::string message) {
        if (current().kind == TokenKind::Identifier || current().kind == TokenKind::Keyword) {
            const auto token = current();
            ++position_;
            return token;
        }
        error(current(), "expected-identifier", std::move(message));
        return std::nullopt;
    }

    void error(const Token& token, std::string code, std::string message) {
        diagnostics_.error("parser", std::move(code), token.location, std::move(message));
    }

    static bool isAccessFlag(std::string_view text) {
        return text == "readable" || text == "writable";
    }

    static bool isTypeStart(std::string_view text, TokenKind kind) {
        if (kind == TokenKind::Identifier) {
            return true;
        }
        return text == "int" || text == "uint" || text == "float" || text == "char" || text == "bool" ||
            text == "ptr" || text == "ref" || text == "fptr" || text == "chain" || text == "void";
    }

    static std::optional<std::string_view> compoundAssignmentOperator(std::string_view text) {
        if (text == "+=") {
            return "+";
        }
        if (text == "-=") {
            return "-";
        }
        if (text == "*=") {
            return "*";
        }
        if (text == "/=") {
            return "/";
        }
        return std::nullopt;
    }

    struct OperatorUse {
        Token op;
        bool authorized = false;
        ExprPtr reason;
    };

    static bool isAllowedOperator(std::string_view text, std::initializer_list<std::string_view> allowed) {
        return std::find(allowed.begin(), allowed.end(), text) != allowed.end();
    }

    std::string parseAccessFlags() {
        std::string access;
        while (isAccessFlag(current().text)) {
            if (!access.empty()) {
                access += ' ';
            }
            access += current().text;
            ++position_;
        }
        return access;
    }

    std::optional<TypeName> parseType() {
        if (!isTypeStart(current().text, current().kind)) {
            error(current(), "expected-type", "expected type");
            return std::nullopt;
        }

        std::ostringstream out;
        int angleDepth = 0;
        bool consumedFirst = false;
        while (!atEnd()) {
            const auto text = current().text;
            if (consumedFirst && angleDepth == 0) {
                if (current().kind == TokenKind::Identifier || text == "," || text == ")" || text == "{" || text == "=" ||
                    text == ";" || text == "code" || text == "requires" || text == "allowed" || text == "where") {
                    break;
                }
            }

            if (text == "<") {
                ++angleDepth;
            } else if (text == ">") {
                --angleDepth;
            }

            if (!out.str().empty()) {
                const bool noSpaceBefore = text == ":" || text == ">" || text == "]" || text == ",";
                const bool noSpaceAfterPrev = out.str().back() == '<' || out.str().back() == ':' || out.str().back() == '[' ||
                    out.str().back() == ',';
                if (!noSpaceBefore && !noSpaceAfterPrev) {
                    out << ' ';
                }
            }
            out << text;
            ++position_;
            consumedFirst = true;

            if (angleDepth <= 0 && (text == "void" || text == "chain")) {
                break;
            }
            if (angleDepth <= 0 && (text == "8" || text == "16" || text == "32" || text == "64" || text == "1")) {
                if (match("[")) {
                    if (!expect("]", "expected ] in array type")) {
                        return std::nullopt;
                    }
                    out << "[]";
                }
                break;
            }
            if (angleDepth <= 0 && current().text == "[") {
                ++position_;
                if (!expect("]", "expected ] in array type")) {
                    return std::nullopt;
                }
                out << "[]";
                break;
            }
            if (angleDepth <= 0 && consumedFirst && tokens_[position_ - 1].kind == TokenKind::Identifier) {
                break;
            }
        }

        return TypeName{out.str()};
    }

    std::optional<FunctionDecl> parseFunction(bool method) {
        FunctionDecl function;
        function.location = current().location;
        if (!expect(method ? "method" : "function", method ? "expected method" : "expected function")) {
            return std::nullopt;
        }

        auto access = expectIdentifier("expected function access modifier");
        if (!access) {
            return std::nullopt;
        }
        function.access = access->text;

        auto category = expectIdentifier("expected function category");
        if (!category) {
            return std::nullopt;
        }
        if (category->text == "approval") {
            function.category = FunctionCategory::Approval;
        } else if (category->text == "uncallable") {
            function.category = FunctionCategory::Uncallable;
        } else {
            function.category = FunctionCategory::Callable;
        }

        if (match("returnable")) {
            function.returnable = true;
        } else if (match("void")) {
            function.returnable = false;
            function.returnType = TypeName{"void"};
        } else {
            error(current(), "expected-returnability", "expected returnable or void");
            return std::nullopt;
        }

        parseAccessFlags();
        if (function.returnType.text.empty()) {
            auto type = parseType();
            if (!type) {
                return std::nullopt;
            }
            function.returnType = std::move(*type);
        }

        auto name = expectIdentifier("expected function name");
        if (!name) {
            return std::nullopt;
        }
        function.name = name->text;

        if (!expect("(", "expected ( before parameter list")) {
            return std::nullopt;
        }
        if (!check(")")) {
            do {
                auto param = parseParam();
                if (!param) {
                    return std::nullopt;
                }
                function.params.push_back(std::move(*param));
            } while (match(","));
        }
        if (!expect(")", "expected ) after parameter list")) {
            return std::nullopt;
        }

        if (!parseFunctionClauses(function)) {
            return std::nullopt;
        }

        function.body = parseBlock();
        if (diagnostics_.hasErrors()) {
            return std::nullopt;
        }
        return function;
    }

    std::optional<Param> parseParam() {
        Param param;
        param.location = current().location;
        param.access = parseAccessFlags();
        auto type = parseType();
        if (!type) {
            return std::nullopt;
        }
        param.type = std::move(*type);
        auto name = expectIdentifier("expected parameter name");
        if (!name) {
            return std::nullopt;
        }
        param.name = name->text;
        return param;
    }

    bool parseFunctionClauses(FunctionDecl& function) {
        while (!atEnd() && !check("{")) {
            if (match("code")) {
                if (!expect("size", "expected size after code")) {
                    return false;
                }
                function.codeSize = parseRequiredNumber("expected code size");
                continue;
            }
            if (match("max")) {
                if (!expect("stack", "expected stack after max") || !expect("size", "expected size after max stack")) {
                    return false;
                }
                function.maxStackSize = parseRequiredNumber("expected max stack size");
                continue;
            }
            if (match("locals")) {
                function.declaredLocalCount = parseRequiredNumber("expected declared local variable count");
                continue;
            }
            if (match("authorized")) {
                if (!expect("by", "expected by after authorized")) {
                    return false;
                }
                auto authority = expectIdentifier("expected definition authority");
                if (!authority) {
                    return false;
                }
                function.definitionAuthority = authority->text;
                continue;
            }
            if (match("requires")) {
                if (match("security")) {
                    if (!expect("level", "expected level after security")) {
                        return false;
                    }
                    function.securityLevel = parseRequiredNumber("expected security level");
                    continue;
                }
                if (match("grantor")) {
                    auto grantor = expectIdentifier("expected grantor name");
                    if (!grantor) {
                        return false;
                    }
                    function.grantor = grantor->text;
                    continue;
                }
                error(current(), "bad-requires-clause", "expected security level or grantor after requires");
                return false;
            }
            if (match("allowed")) {
                if (!expect("roles", "expected roles after allowed")) {
                    return false;
                }
                do {
                    auto role = expectIdentifier("expected role name");
                    if (!role) {
                        return false;
                    }
                    function.allowedRoles.push_back(role->text);
                } while (match(","));
                continue;
            }
            if (match("where")) {
                if (!expect("callable", "expected callable after where") || !expect("from", "expected from after where callable")) {
                    return false;
                }
                do {
                    auto caller = expectIdentifier("expected caller name");
                    if (!caller) {
                        return false;
                    }
                    function.callableFrom.push_back(caller->text);
                } while (match(","));
                continue;
            }
            error(current(), "unexpected-function-clause", "unexpected token in function header");
            return false;
        }
        return true;
    }

    int parseRequiredNumber(std::string message) {
        if (current().kind != TokenKind::Number) {
            error(current(), "expected-number", std::move(message));
            return 0;
        }
        int value = 0;
        std::from_chars(current().text.data(), current().text.data() + current().text.size(), value);
        ++position_;
        return value;
    }

    std::vector<StatementPtr> parseBlock() {
        std::vector<StatementPtr> statements;
        if (!expect("{", "expected { before block")) {
            return statements;
        }
        while (!atEnd() && !check("}")) {
            auto statement = parseStatement();
            if (!statement) {
                synchronizeStatement();
                if (diagnostics_.hasErrors()) {
                    return statements;
                }
                continue;
            }
            statements.push_back(std::move(statement));
        }
        expect("}", "expected } after block");
        return statements;
    }

    StatementPtr parseStatement() {
        bool proceeded = match("proceed");
        const auto start = current().location;
        StatementPtr statement;
        if (check("declare")) {
            statement = parseVarDecl();
        } else if (check("assign")) {
            statement = parseAssign();
        } else if (check("return")) {
            statement = parseReturn();
        } else if (check("verifyfunctionidentity")) {
            statement = parseVerify();
        } else if (check("authorize")) {
            statement = parseAuthorizeCall();
        } else if (check("prove")) {
            statement = parseProof();
        } else if (check("operatorinput")) {
            statement = parseOperatorInput();
        } else if (checkConditionalStart()) {
            statement = parseIf();
        } else if (check("while")) {
            statement = parseWhile();
        } else if (check("do")) {
            statement = parseDoUntil();
        } else if (check("for")) {
            statement = parseFor();
        } else if (check("gate")) {
            statement = makeStatement(StatementKind::Gate, start);
            statement->expression = parseGateExpr();
            if (!statement->expression) {
                return nullptr;
            }
        } else if (check("instantiate")) {
            statement = parseInstantiate();
        } else if (check("grant") || check("delegate") || check("revoke") || check("assume") || check("trace")) {
            statement = parseRoleStmt();
        } else if (match("break")) {
            expect(";", "expected ; after break");
            statement = makeStatement(StatementKind::Break, start);
        } else if (match("continue")) {
            expect(";", "expected ; after continue");
            statement = makeStatement(StatementKind::Continue, start);
        } else if (match("clockcycle")) {
            expect(";", "expected ; after clockcycle");
            statement = makeStatement(StatementKind::ClockCycle, start);
        } else if (match("yieldgate")) {
            auto id = expectIdentifier("expected yielded gate name");
            expect(";", "expected ; after yieldgate");
            statement = makeStatement(StatementKind::YieldGate, start);
            if (id) {
                statement->identifier = id->text;
            }
        } else {
            error(current(), "unexpected-statement", "unexpected statement");
            return nullptr;
        }
        if (statement) {
            statement->proceeded = proceeded;
        }
        return statement;
    }

    ExprPtr parseBecauseLiteral(std::string_view owner) {
        const auto start = current().location;
        if (!expect("because", "expected because before required justification")) {
            return nullptr;
        }
        if (!expect("literal", "expected literal in required justification")) {
            return nullptr;
        }
        const auto literalToken = current();
        if (!matchKind(TokenKind::String)) {
            error(current(), "expected-literal-string", "expected literal string justification");
            return nullptr;
        }
        auto literal = makeExpr(ExprKind::String, start, literalToken.text);
        literal->declaredLiteral = true;
        if (literal->text.empty()) {
            diagnostics_.error("parser", "empty-justification", start, std::string(owner) + " justification must not be empty");
            return nullptr;
        }
        return literal;
    }

    StatementPtr parseVarDecl() {
        auto statement = makeStatement(StatementKind::VarDecl, current().location);
        statement->varDecl.location = current().location;
        expect("declare", "expected declare");
        if (match("mutable")) {
            statement->varDecl.immutable = false;
        } else if (match("immutable")) {
            statement->varDecl.immutable = true;
        } else {
            error(current(), "expected-mutability", "expected mutable or immutable");
            return nullptr;
        }
        statement->varDecl.access = parseAccessFlags();
        if (!expect("purpose", "expected purpose attribute")) {
            return nullptr;
        }
        auto purpose = expectIdentifier("expected purpose name");
        if (!purpose) {
            return nullptr;
        }
        statement->varDecl.purpose = purpose->text;
        if (!expect("scope", "expected scope attribute")) {
            return nullptr;
        }
        auto scope = expectIdentifier("expected scope name");
        if (!scope) {
            return nullptr;
        }
        statement->varDecl.scope = scope->text;
        if (match("volatile")) {
            statement->varDecl.isVolatile = true;
        }
        auto type = parseType();
        if (!type) {
            return nullptr;
        }
        statement->varDecl.type = std::move(*type);
        auto name = expectIdentifier("expected variable name");
        if (!name) {
            return nullptr;
        }
        statement->varDecl.name = name->text;
        if (check("{")) {
            if (!validateEventBlockAndSkip()) {
                return nullptr;
            }
        }
        if (match("=")) {
            statement->varDecl.initializer = parseExpression();
            if (!statement->varDecl.initializer) {
                return nullptr;
            }
        }
        if (!expect(";", "expected ; after declaration")) {
            return nullptr;
        }
        return statement;
    }

    StatementPtr parseAssign() {
        auto statement = makeStatement(StatementKind::Assign, current().location);
        statement->assign.location = current().location;
        expect("assign", "expected assign");
        statement->assign.target = parseExpression();
        if (!statement->assign.target) {
            return nullptr;
        }
        if (const auto op = compoundAssignmentOperator(current().text)) {
            const auto opToken = current();
            ++position_;
            auto right = parseExpression();
            if (!right) {
                return nullptr;
            }
            auto left = cloneExpr(*statement->assign.target);
            auto value = makeExpr(ExprKind::Binary, opToken.location, std::string{*op});
            value->left = std::move(left);
            value->right = std::move(right);
            statement->assign.value = std::move(value);
            if (!expect(";", "expected ; after compound assignment")) {
                return nullptr;
            }
            return statement;
        }
        if (!expect("=", "expected = in assignment")) {
            return nullptr;
        }
        if (check("authorize")) {
            auto call = parseAuthorizeCall();
            if (!call) {
                return nullptr;
            }
            statement->kind = StatementKind::AssignFromCall;
            statement->authorizeCall = std::move(call->authorizeCall);
            return statement;
        }
        statement->assign.value = parseExpression();
        if (!statement->assign.value || !expect(";", "expected ; after assignment")) {
            return nullptr;
        }
        return statement;
    }

    StatementPtr parseReturn() {
        auto statement = makeStatement(StatementKind::Return, current().location);
        expect("return", "expected return");
        if (!check(";")) {
            statement->expression = parseExpression();
            if (!statement->expression) {
                return nullptr;
            }
        }
        expect(";", "expected ; after return");
        return statement;
    }

    StatementPtr parseVerify() {
        auto statement = makeStatement(StatementKind::VerifyFunctionIdentity, current().location);
        expect("verifyfunctionidentity", "expected verifyfunctionidentity");
        expect("(", "expected (");
        expect(")", "expected )");
        expect(";", "expected ; after verifyfunctionidentity");
        return statement;
    }

    StatementPtr parseAuthorizeCall() {
        auto statement = makeStatement(StatementKind::AuthorizeCall, current().location);
        statement->authorizeCall.location = current().location;
        if (!parseAuthorizePayload(*statement, true)) {
            return nullptr;
        }
        return statement;
    }

    ExprPtr parseAuthorizeCallExpr() {
        const auto start = current().location;
        auto statement = makeStatement(StatementKind::AuthorizeCall, current().location);
        statement->authorizeCall.location = current().location;
        if (!parseAuthorizePayload(*statement, false)) {
            return nullptr;
        }
        if (statement->kind == StatementKind::FptrAssign) {
            error(current(), "fptr-assignment-not-expression", "function pointer assignment cannot be used as an expression");
            return nullptr;
        }
        if (statement->authorizeCall.discardingReturn) {
            error(current(), "discarding-return-in-expression", "call expressions cannot discard their return value");
            return nullptr;
        }
        auto expr = makeExpr(ExprKind::AuthorizeCall, start);
        expr->authorizeCall = std::move(statement->authorizeCall);
        return expr;
    }

    bool parseAuthorizePayload(Statement& statement, bool requireSemicolon) {
        expect("authorize", "expected authorize");
        if (match("invocation")) {
            if (match("of")) {
                // Direct function invocation.
            } else if (match("via")) {
                statement.authorizeCall.viaPointer = true;
            } else {
                error(current(), "bad-authorize", "expected of or via after authorize invocation");
                return false;
            }
        } else if (match("fptr")) {
            if (!requireSemicolon) {
                error(current(), "fptr-assignment-not-expression", "function pointer assignment cannot be used as an expression");
                return false;
            }
            statement.kind = StatementKind::FptrAssign;
            statement.fptrAssign.location = statement.location;
            if (!expect("assignment", "expected assignment after authorize fptr")) {
                return false;
            }
            if (!expect("of", "expected of after fptr assignment")) {
                return false;
            }
            auto variable = expectIdentifier("expected function pointer variable");
            if (!variable) {
                return false;
            }
            statement.fptrAssign.variable = variable->text;
            if (!expect("to", "expected to in fptr assignment")) {
                return false;
            }
            auto target = parseQualifiedName();
            if (!target) {
                return false;
            }
            statement.fptrAssign.target = *target;
            while (!atEnd() && !check(";")) {
                if (match("with") && match("authority")) {
                    if (!expect("chain", "expected chain after authority")) {
                        return false;
                    }
                    auto chain = expectIdentifier("expected authority chain identifier");
                    if (!chain) {
                        return false;
                    }
                    statement.fptrAssign.authorityChain = chain->text;
                    continue;
                }
                ++position_;
            }
            expect(";", "expected ; after fptr statement");
            return true;
        } else {
            error(current(), "bad-authorize", "expected invocation after authorize");
            return false;
        }

        auto target = parseQualifiedName();
        if (!target) {
            return false;
        }
        statement.authorizeCall.target = *target;

        while (!atEnd() && !check(";") && (requireSemicolon || !isExpressionDelimiter())) {
            if (match("at")) {
                if (!expect("security", "expected security after at") || !expect("level", "expected level after security")) {
                    return false;
                }
                statement.authorizeCall.securityLevel = parseRequiredNumber("expected security level");
                continue;
            }
            if (match("predictstackdepth")) {
                statement.authorizeCall.predictStackDepth = parseRequiredNumber("expected predicted stack depth");
                continue;
            }
            if (match("nullcheck")) {
                statement.authorizeCall.hasNullCheck = true;
                if (match("true")) {
                    statement.authorizeCall.nullCheck = true;
                } else if (match("false")) {
                    statement.authorizeCall.nullCheck = false;
                } else {
                    error(current(), "expected-nullcheck-value", "expected true or false after nullcheck");
                    return false;
                }
                continue;
            }
            if (match("with")) {
                if (match("arguments")) {
                    if (!parseCallArguments(statement.authorizeCall)) {
                        return false;
                    }
                    continue;
                }
                if (match("io")) {
                    if (!parseIoClause(statement.authorizeCall.io)) {
                        return false;
                    }
                    continue;
                }
                if (match("memory")) {
                    if (!expect("limit", "expected limit after memory")) {
                        return false;
                    }
                    statement.authorizeCall.memoryLimit = parseRequiredNumber("expected memory limit");
                    continue;
                }
                if (match("timeout")) {
                    statement.authorizeCall.timeout = parseRequiredNumber("expected timeout");
                    continue;
                }
                if (match("authority")) {
                    if (!expect("chain", "expected chain after authority")) {
                        return false;
                    }
                    auto chain = expectIdentifier("expected authority chain identifier");
                    if (!chain) {
                        return false;
                    }
                    statement.authorizeCall.authorityChain = chain->text;
                    continue;
                }
                if (match("approval")) {
                    if (match("of")) {
                        auto approval = parseQualifiedName();
                        if (!approval) {
                            return false;
                        }
                        statement.authorizeCall.approvalFunction = *approval;
                        continue;
                    }
                    if (match("timeout")) {
                        statement.authorizeCall.approvalTimeout = parseRequiredNumber("expected approval timeout");
                        if (!requireSemicolon) {
                            break;
                        }
                        continue;
                    }
                    if (match("justification")) {
                        if (current().kind == TokenKind::String) {
                            ++position_;
                            continue;
                        }
                        error(current(), "expected-approval-justification", "expected approval justification string");
                        return false;
                    }
                    continue;
                }
                if (match("justification")) {
                    if (current().kind == TokenKind::String) {
                        ++position_;
                        continue;
                    }
                    error(current(), "expected-justification", "expected justification string");
                    return false;
                }
            }
            if (match("discarding")) {
                expect("return", "expected return after discarding");
                statement.authorizeCall.discardingReturn = true;
                continue;
            }
            ++position_;
        }
        if (requireSemicolon) {
            return expect(";", "expected ; after authorized invocation");
        }
        return true;
    }

    StatementPtr parseProof() {
        auto statement = makeStatement(StatementKind::Proof, current().location);
        statement->proof.location = current().location;
        if (!expect("prove", "expected prove")) {
            return nullptr;
        }
        auto form = expectIdentifier("expected proof form");
        if (!form) {
            return nullptr;
        }
        if (form->text != "theorem" && form->text != "axiom") {
            error(*form, "expected-proof-form", "expected theorem or axiom after prove");
            return nullptr;
        }
        statement->proof.form = form->text;
        auto name = expectIdentifier("expected proof obligation name");
        if (!name) {
            return nullptr;
        }
        statement->proof.name = name->text;
        if (!expect(":", "expected : after proof obligation name")) {
            return nullptr;
        }

        if (statement->proof.form == "theorem") {
            if (!(match("require") || match("requires"))) {
                error(current(), "expected-proof-premise", "expected require before theorem premise");
                return nullptr;
            }
            if (!expect("(", "expected ( before theorem premise")) {
                return nullptr;
            }
            statement->proof.premise = parseExpression();
            if (!statement->proof.premise || !expect(")", "expected ) after theorem premise")) {
                return nullptr;
            }
            if (!(match("therefore") || match("ensures"))) {
                error(current(), "expected-proof-claim", "expected therefore before theorem claim");
                return nullptr;
            }
            if (!expect("(", "expected ( before theorem claim")) {
                return nullptr;
            }
            statement->proof.claim = parseExpression();
            if (!statement->proof.claim || !expect(")", "expected ) after theorem claim")) {
                return nullptr;
            }
        } else {
            if (!expect("assume", "expected assume before axiom claim") || !expect("(", "expected ( before axiom claim")) {
                return nullptr;
            }
            statement->proof.claim = parseExpression();
            if (!statement->proof.claim || !expect(")", "expected ) after axiom claim")) {
                return nullptr;
            }
        }

        statement->proof.reason = parseBecauseLiteral("proof");
        if (!statement->proof.reason || !expect(";", "expected ; after proof obligation")) {
            return nullptr;
        }
        return statement;
    }

    bool isExpressionDelimiter() const {
        return check(",") || check("}") || check(")");
    }

    bool parseIoClause(IoClause& io) {
        io.present = true;
        io.location = previous().location;
        if (!expect("operation", "expected operation after io")) {
            return false;
        }
        io.reason = parseBecauseLiteral("io operation");
        return io.reason != nullptr;
    }

    std::optional<std::string> parseQualifiedName() {
        auto first = expectIdentifier("expected qualified name");
        if (!first) {
            return std::nullopt;
        }
        std::string name = first->text;
        while (match(".")) {
            auto part = expectIdentifier("expected name after .");
            if (!part) {
                return std::nullopt;
            }
            name += ".";
            name += part->text;
        }
        return name;
    }

    bool parseCallArguments(AuthorizeCall& call) {
        if (!expect("{", "expected { before arguments")) {
            return false;
        }
        if (!check("}")) {
            do {
                FunctionCallArg arg;
                arg.location = current().location;
                auto name = expectIdentifier("expected argument name");
                if (!name) {
                    return false;
                }
                arg.name = name->text;
                if (!expect("by", "expected by in argument")) {
                    return false;
                }
                if (match("reference")) {
                    arg.byReference = true;
                } else if (match("value")) {
                    arg.byReference = false;
                } else {
                    error(current(), "expected-pass-mode", "expected value or reference");
                    return false;
                }
                if (!expect("=", "expected = in argument")) {
                    return false;
                }
                arg.value = parseExpression();
                if (!arg.value) {
                    return false;
                }
                call.arguments.push_back(std::move(arg));
            } while (match(","));
        }
        return expect("}", "expected } after arguments");
    }

    StatementPtr parseOperatorInput() {
        auto statement = makeStatement(StatementKind::OperatorInput, current().location);
        expect("operatorinput", "expected operatorinput");
        expect("prompt", "expected prompt");
        if (current().kind == TokenKind::String || current().kind == TokenKind::Identifier || current().kind == TokenKind::Keyword) {
            ++position_;
        } else {
            error(current(), "expected-prompt", "expected prompt string or identifier");
            return nullptr;
        }
        expect("timeout", "expected timeout");
        parseRequiredNumber("expected timeout milliseconds");
        expect("into", "expected into");
        auto output = expectIdentifier("expected output variable");
        if (!output) {
            return nullptr;
        }
        statement->identifier = output->text;
        if (match("with")) {
            if (!match("io")) {
                error(current(), "expected-io-clause", "expected io operation declaration after operatorinput");
                return nullptr;
            }
            if (!parseIoClause(statement->io)) {
                return nullptr;
            }
        }
        expect(";", "expected ; after operatorinput");
        return statement;
    }

    StatementPtr parseInstantiate() {
        auto statement = makeStatement(StatementKind::Instantiate, current().location);
        statement->instantiate.location = current().location;
        if (!expect("instantiate", "expected instantiate")) {
            return nullptr;
        }
        auto className = expectIdentifier("expected class name after instantiate");
        if (!className) {
            return nullptr;
        }
        statement->instantiate.className = className->text;
        auto instanceName = expectIdentifier("expected instance name after class name");
        if (!instanceName) {
            return nullptr;
        }
        statement->instantiate.name = instanceName->text;
        if (match("authorized")) {
            if (!expect("by", "expected by after authorized")) {
                return nullptr;
            }
            auto authority = expectIdentifier("expected instantiation authority");
            if (!authority) {
                return nullptr;
            }
            statement->instantiate.authority = authority->text;
        }
        if (!expect("memory", "expected memory clause for instance")) {
            return nullptr;
        }
        statement->instantiate.memoryAccess = parseAccessFlags();
        if (!expect("from", "expected abstract factory source")) {
            return nullptr;
        }
        auto factory = expectIdentifier("expected abstract factory name");
        if (!factory) {
            return nullptr;
        }
        statement->instantiate.factory = factory->text;
        if (!expect("using", "expected using before strategy") || !expect("strategy", "expected strategy keyword")) {
            return nullptr;
        }
        auto strategy = expectIdentifier("expected strategy name");
        if (!strategy) {
            return nullptr;
        }
        statement->instantiate.strategy = strategy->text;
        if (!expect("injecting", "expected injecting before dependency")) {
            return nullptr;
        }
        auto dependency = expectIdentifier("expected dependency name");
        if (!dependency) {
            return nullptr;
        }
        statement->instantiate.dependency = dependency->text;
        statement->instantiate.reason = parseBecauseLiteral("instance");
        if (!statement->instantiate.reason || !expect(";", "expected ; after instantiate")) {
            return nullptr;
        }
        return statement;
    }

    StatementPtr parseIf() {
        auto statement = makeStatement(StatementKind::If, current().location);
        if (!parseConditionalPreamble()) {
            return nullptr;
        }
        statement->expression = parseExpression();
        if (!statement->expression || !expect(")", "expected ) after if condition")) {
            return nullptr;
        }
        if (!parseJudgmentClause(statement->judgment, true)) {
            return nullptr;
        }
        statement->thenBody = parseBlock();
        if (match("else")) {
            if (checkConditionalStart()) {
                statement->elseBody.push_back(parseIf());
                if (!statement->elseBody.back()) {
                    return nullptr;
                }
            } else {
                if (!parseJudgmentClause(statement->elseJudgment, false)) {
                    return nullptr;
                }
                statement->elseBody = parseBlock();
            }
        }
        return statement;
    }

    bool checkConditionalStart() const {
        return check("if") || check("convene");
    }

    bool parseConditionalPreamble() {
        if (match("if")) {
            return expect("(", "expected ( after if");
        }
        if (match("convene")) {
            return expect("conditional", "expected conditional after convene") &&
                expect("tribunal", "expected tribunal after convene conditional") &&
                expect("over", "expected over after convene conditional tribunal") &&
                expect("(", "expected ( after convene conditional tribunal over");
        }
        error(current(), "expected-conditional-start", "expected conditional tribunal");
        return false;
    }

    bool parseJudgmentClause(JudgmentClause& judgment, bool requireCounts) {
        judgment.location = current().location;
        judgment.present = true;
        if (!expect("judging", "expected judging clause before conditional block")) {
            return false;
        }
        if (!expect("authorized", "expected authorized in judging clause") || !expect("by", "expected by in judging clause")) {
            return false;
        }
        auto authority = expectIdentifier("expected judging authority");
        if (!authority) {
            return false;
        }
        judgment.authority = authority->text;
        judgment.reason = parseBecauseLiteral("judgment");
        if (!judgment.reason) {
            return false;
        }
        if (!requireCounts) {
            return true;
        }
        if (!expect("expects", "expected expects in judging clause")) {
            return false;
        }
        if (!match("elseifs") && !match("appealroutes")) {
            error(current(), "expected-elseif-count-label", "expected elseifs or appealroutes count in judging clause");
            return false;
        }
        judgment.declaredElseIfCount = parseRequiredNumber("expected declared else-if count");
        if (!match("else") && !match("defaultdockets")) {
            error(current(), "expected-else-count-label", "expected else or defaultdockets count in judging clause");
            return false;
        }
        judgment.declaredElseCount = parseRequiredNumber("expected declared else count");
        return true;
    }

    StatementPtr parseWhile() {
        auto statement = makeStatement(StatementKind::While, current().location);
        expect("while", "expected while");
        expect("(", "expected ( after while");
        statement->expression = parseExpression();
        if (!statement->expression || !expect(")", "expected ) after while condition")) {
            return nullptr;
        }
        if (!parseFormalClauses(*statement)) {
            return nullptr;
        }
        statement->thenBody = parseBlock();
        return statement;
    }

    StatementPtr parseDoUntil() {
        auto statement = makeStatement(StatementKind::DoUntil, current().location);
        expect("do", "expected do");
        if (!parseFormalClauses(*statement)) {
            return nullptr;
        }
        statement->thenBody = parseBlock();
        if (!expect("until", "expected until after do block") || !expect("(", "expected ( after until")) {
            return nullptr;
        }
        statement->expression = parseExpression();
        if (!statement->expression || !expect(")", "expected ) after until condition") ||
            !expect(";", "expected ; after do until")) {
            return nullptr;
        }
        return statement;
    }

    StatementPtr parseFor() {
        auto statement = makeStatement(StatementKind::For, current().location);
        expect("for", "expected for");
        expect("(", "expected ( after for");
        if (!check(";")) {
            statement->initializer = parseExpression();
            if (!statement->initializer) {
                return nullptr;
            }
        }
        if (!expect(";", "expected ; after for initializer")) {
            return nullptr;
        }
        if (!check(";")) {
            statement->expression = parseExpression();
            if (!statement->expression) {
                return nullptr;
            }
        }
        if (!expect(";", "expected ; after for condition")) {
            return nullptr;
        }
        if (!check(")")) {
            statement->increment = parseExpression();
            if (!statement->increment) {
                return nullptr;
            }
        }
        if (!expect(")", "expected ) after for clauses")) {
            return nullptr;
        }
        if (!parseFormalClauses(*statement)) {
            return nullptr;
        }
        statement->thenBody = parseBlock();
        return statement;
    }

    bool parseFormalClauses(Statement& statement) {
        while (check("invariant") || check("decreases")) {
            FormalClause clause;
            clause.location = current().location;
            clause.kind = current().text;
            ++position_;
            if (!expect("(", "expected ( before formal clause expression")) {
                return false;
            }
            clause.expression = parseExpression();
            if (!clause.expression || !expect(")", "expected ) after formal clause expression")) {
                return false;
            }
            clause.reason = parseBecauseLiteral("formal clause");
            if (!clause.reason) {
                return false;
            }
            statement.formalClauses.push_back(std::move(clause));
        }
        return true;
    }

    StatementPtr parseRoleStmt() {
        auto statement = makeStatement(StatementKind::Role, current().location);
        statement->role.location = current().location;
        statement->role.op = current().text;
        if (match("grant")) {
            auto role = expectIdentifier("expected role to grant");
            if (!role || !expect("to", "expected to in grant statement")) {
                return nullptr;
            }
            auto principal = expectIdentifier("expected grant principal");
            if (!principal || !expect("via", "expected via in grant statement")) {
                return nullptr;
            }
            auto chain = expectIdentifier("expected authority chain");
            if (!chain || !expect(";", "expected ; after grant")) {
                return nullptr;
            }
            statement->role.role = role->text;
            statement->role.principal = principal->text;
            statement->role.chain = chain->text;
            return statement;
        }
        if (match("delegate")) {
            auto role = expectIdentifier("expected role to delegate");
            if (!role || !expect("to", "expected to in delegate statement")) {
                return nullptr;
            }
            auto principal = expectIdentifier("expected delegate principal");
            if (!principal || !expect("with", "expected with in delegate statement")) {
                return nullptr;
            }
            auto chain = expectIdentifier("expected authority chain");
            if (!chain || !expect(";", "expected ; after delegate")) {
                return nullptr;
            }
            statement->role.role = role->text;
            statement->role.principal = principal->text;
            statement->role.chain = chain->text;
            return statement;
        }
        if (match("revoke")) {
            auto role = expectIdentifier("expected role to revoke");
            if (!role || !expect("from", "expected from in revoke statement")) {
                return nullptr;
            }
            auto principal = expectIdentifier("expected revoke principal");
            if (!principal || !expect(";", "expected ; after revoke")) {
                return nullptr;
            }
            statement->role.role = role->text;
            statement->role.principal = principal->text;
            return statement;
        }
        if (match("assume")) {
            auto role = expectIdentifier("expected role to assume");
            if (!role || !expect("with", "expected with in assume statement")) {
                return nullptr;
            }
            auto chain = expectIdentifier("expected authority chain");
            if (!chain || !expect(";", "expected ; after assume")) {
                return nullptr;
            }
            statement->role.role = role->text;
            statement->role.chain = chain->text;
            return statement;
        }
        if (match("trace")) {
            if (!expect("to", "expected to after trace")) {
                return nullptr;
            }
            auto chain = expectIdentifier("expected chain to trace");
            if (!chain || !expect(";", "expected ; after trace")) {
                return nullptr;
            }
            statement->role.chain = chain->text;
            return statement;
        }
        error(current(), "expected-role-statement", "expected role statement");
        return nullptr;
    }

    StatementPtr parseSkippedStatement() {
        auto statement = makeStatement(StatementKind::Unknown, current().location);
        int depth = 0;
        do {
            if (current().text == "{") {
                ++depth;
            } else if (current().text == "}") {
                if (depth == 0) {
                    break;
                }
                --depth;
            }
            if (depth == 0 && current().text == ";") {
                ++position_;
                break;
            }
            ++position_;
        } while (!atEnd());
        return statement;
    }

    ExprPtr parseExpression() {
        if (match("compute")) {
            const auto start = previous().location;
            expect("(", "expected ( after compute");
            auto inner = parseExpression();
            if (!inner || !expect(")", "expected ) after compute expression")) {
                return nullptr;
            }
            if (!expect("with", "expected with after compute") || !expect("overflow", "expected overflow policy")) {
                return nullptr;
            }
            auto policy = expectIdentifier("expected overflow policy");
            if (!policy) {
                return nullptr;
            }
            auto compute = makeExpr(ExprKind::Compute, start, policy->text);
            compute->left = std::move(inner);
            return compute;
        }
        if (check("gate")) {
            return parseGateExpr();
        }
        return parseConditional();
    }

    ExprPtr parseConditional() {
        auto condition = parseLogicalOr();
        if (!condition || !match("?")) {
            return condition;
        }
        const auto start = previous().location;
        auto whenTrue = parseExpression();
        if (!whenTrue || !expect(":", "expected : in conditional expression")) {
            return nullptr;
        }
        auto whenFalse = parseConditional();
        if (!whenFalse) {
            return nullptr;
        }
        auto expr = makeExpr(ExprKind::Conditional, start);
        expr->left = std::move(condition);
        expr->right = std::move(whenTrue);
        expr->third = std::move(whenFalse);
        return expr;
    }

    ExprPtr parseLogicalOr() {
        auto expr = parseLogicalAnd();
        while (match("||") || match("or")) {
            const auto op = previous();
            auto right = parseLogicalAnd();
            expr = makeBinary(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    ExprPtr parseLogicalAnd() {
        auto expr = parseEquality();
        while (match("&&") || match("and")) {
            const auto op = previous();
            auto right = parseEquality();
            expr = makeBinary(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::optional<OperatorUse> parseOperatorUse(std::initializer_list<std::string_view> allowed) {
        if (check("authorize")) {
            if (position_ + 3 >= tokens_.size() || tokens_[position_ + 1].text != "use" ||
                tokens_[position_ + 2].text != "operator" ||
                !isAllowedOperator(tokens_[position_ + 3].text, allowed)) {
                return std::nullopt;
            }
            position_ += 3;
            if (!isAllowedOperator(current().text, allowed)) {
                error(current(), "expected-authorized-operator", "expected authorized comparison operator");
                return std::nullopt;
            }
            OperatorUse use;
            use.op = current();
            use.authorized = true;
            ++position_;
            return use;
        }
        for (const auto op : allowed) {
            if (match(op)) {
                return OperatorUse{previous(), false, nullptr};
            }
        }
        return std::nullopt;
    }

    ExprPtr parseEquality() {
        auto expr = parseRelational();
        while (true) {
            auto op = parseOperatorUse({"==", "!="});
            if (!op) {
                break;
            }
            auto right = parseRelational();
            if (!right) {
                return nullptr;
            }
            if (op->authorized) {
                op->reason = parseBecauseLiteral("operator authorization");
                if (!op->reason) {
                    return nullptr;
                }
            }
            auto binary = makeBinary(std::move(expr), op->op, std::move(right));
            binary->operatorAuthorized = op->authorized;
            binary->operatorReason = std::move(op->reason);
            expr = std::move(binary);
        }
        return expr;
    }

    ExprPtr parseRelational() {
        auto expr = parseAdditive();
        while (true) {
            auto op = parseOperatorUse({"<", ">", "<=", ">="});
            if (!op) {
                break;
            }
            auto right = parseAdditive();
            if (!right) {
                return nullptr;
            }
            if (op->authorized) {
                op->reason = parseBecauseLiteral("operator authorization");
                if (!op->reason) {
                    return nullptr;
                }
            }
            auto binary = makeBinary(std::move(expr), op->op, std::move(right));
            binary->operatorAuthorized = op->authorized;
            binary->operatorReason = std::move(op->reason);
            expr = std::move(binary);
        }
        return expr;
    }

    ExprPtr parseAdditive() {
        auto expr = parseMultiplicative();
        while (match("+") || match("-")) {
            const auto op = previous();
            auto right = parseMultiplicative();
            expr = makeBinary(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    ExprPtr parseMultiplicative() {
        auto expr = parseUnary();
        while (match("*") || match("/")) {
            const auto op = previous();
            auto right = parseUnary();
            expr = makeBinary(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    ExprPtr parseUnary() {
        if (match("&") || match("*") || match("!") || match("not")) {
            const auto op = previous();
            auto expr = makeExpr(ExprKind::Unary, op.location, op.text);
            expr->left = parseUnary();
            return expr;
        }
        return parsePostfix();
    }

    ExprPtr parsePostfix() {
        auto expr = parsePrimary();
        while (expr && match(".")) {
            auto field = expectIdentifier("expected field name");
            if (!field) {
                return nullptr;
            }
            auto access = makeExpr(ExprKind::Field, field->location, field->text);
            access->left = std::move(expr);
            expr = std::move(access);
        }
        return expr;
    }

    ExprPtr parsePrimary() {
        const auto token = current();
        if (matchKind(TokenKind::Number)) {
            return makeExpr(ExprKind::Integer, token.location, token.text);
        }
        if (matchKind(TokenKind::String)) {
            return makeExpr(ExprKind::String, token.location, token.text);
        }
        if (match("literal")) {
            const auto literalToken = current();
            if (!matchKind(TokenKind::String)) {
                error(current(), "expected-literal-string", "expected string after literal");
                return nullptr;
            }
            auto literal = makeExpr(ExprKind::String, literalToken.location, literalToken.text);
            literal->declaredLiteral = true;
            return literal;
        }
        if (match("true") || match("false")) {
            return makeExpr(ExprKind::Boolean, token.location, token.text);
        }
        if (match("null")) {
            return makeExpr(ExprKind::Null, token.location, token.text);
        }
        if (check("authorize")) {
            return parseAuthorizeCallExpr();
        }
        if (current().kind == TokenKind::Identifier || current().kind == TokenKind::Keyword) {
            ++position_;
            return makeExpr(ExprKind::Identifier, token.location, token.text);
        }
        if (match("(")) {
            auto expr = parseExpression();
            expect(")", "expected ) after expression");
            return expr;
        }
        error(current(), "expected-expression", "expected expression");
        return nullptr;
    }

    bool isGateOpName(std::string_view text) const {
        return text == "and" || text == "or" || text == "not" || text == "xor" || text == "nand" || text == "nor";
    }

    ExprPtr parseGateExpr() {
        const auto start = current().location;
        if (!expect("gate", "expected gate") || !expect("{", "expected { after gate")) {
            return nullptr;
        }
        auto gate = makeExpr(ExprKind::Gate, start);
        while (!atEnd() && !check("}")) {
            if (match("wire")) {
                while (!atEnd() && !check(";")) {
                    ++position_;
                }
                if (!expect(";", "expected ; after wire declaration")) {
                    return nullptr;
                }
                continue;
            }
            if (match("yield")) {
                auto yielded = expectIdentifier("expected yielded wire name");
                if (!yielded) {
                    return nullptr;
                }
                gate->text = yielded->text;
                if (!expect(";", "expected ; after gate yield")) {
                    return nullptr;
                }
                continue;
            }
            if (!isGateOpName(current().text)) {
                error(current(), "expected-gate-op", "expected gate operation");
                return nullptr;
            }
            GateOp op;
            op.location = current().location;
            op.op = current().text;
            ++position_;
            if (!expect("(", "expected ( after gate operation")) {
                return nullptr;
            }
            auto left = expectIdentifier("expected first gate operand");
            if (!left || !expect(",", "expected , after first gate operand")) {
                return nullptr;
            }
            auto right = expectIdentifier("expected second gate operand");
            if (!right) {
                return nullptr;
            }
            op.left = left->text;
            op.right = right->text;
            if (match(",")) {
                auto output = expectIdentifier("expected gate output wire");
                if (!output) {
                    return nullptr;
                }
                op.output = output->text;
            } else {
                op.output = op.right;
            }
            if (!expect(")", "expected ) after gate operation") || !expect(";", "expected ; after gate operation")) {
                return nullptr;
            }
            gate->gateOps.push_back(std::move(op));
        }
        if (!expect("}", "expected } after gate")) {
            return nullptr;
        }
        if (gate->text.empty()) {
            diagnostics_.error("parser", "missing-gate-yield", start, "gate expression must yield a wire");
            return nullptr;
        }
        return gate;
    }

    ExprPtr makeBinary(ExprPtr left, const Token& op, ExprPtr right) {
        auto expr = makeExpr(ExprKind::Binary, op.location, op.text);
        expr->left = std::move(left);
        expr->right = std::move(right);
        return expr;
    }

    std::optional<StructDecl> parseStruct() {
        StructDecl structure;
        structure.location = current().location;
        expect("struct", "expected struct");
        auto name = expectIdentifier("expected struct name");
        if (!name) {
            return std::nullopt;
        }
        structure.name = name->text;
        if (match("fields")) {
            structure.declaredFieldCount = parseRequiredNumber("expected declared struct field count");
        }
        if (match("methods")) {
            structure.declaredMethodCount = parseRequiredNumber("expected declared struct method count");
        }
        if (match("authorized")) {
            if (!expect("by", "expected by after authorized")) {
                return std::nullopt;
            }
            auto authority = expectIdentifier("expected struct definition authority");
            if (!authority) {
                return std::nullopt;
            }
            structure.definitionAuthority = authority->text;
        }
        if (!expect("=", "expected = in struct declaration") || !expect("struct", "expected struct body")) {
            return std::nullopt;
        }
        if (!parseFieldBlock(structure, "struct")) {
            return std::nullopt;
        }
        if (!parseImplementBlock(structure)) {
            return std::nullopt;
        }
        return structure;
    }

    std::optional<StructDecl> parseClass() {
        StructDecl classDecl;
        classDecl.isClass = true;
        classDecl.location = current().location;
        expect("class", "expected class");
        auto name = expectIdentifier("expected class name");
        if (!name) {
            return std::nullopt;
        }
        classDecl.name = name->text;
        if (match("fields")) {
            classDecl.declaredFieldCount = parseRequiredNumber("expected declared class field count");
        }
        if (match("methods")) {
            classDecl.declaredMethodCount = parseRequiredNumber("expected declared class method count");
        }
        if (match("authorized")) {
            if (!expect("by", "expected by after authorized")) {
                return std::nullopt;
            }
            auto authority = expectIdentifier("expected class definition authority");
            if (!authority) {
                return std::nullopt;
            }
            classDecl.definitionAuthority = authority->text;
        }
        classDecl.definitionReason = parseBecauseLiteral("class");
        if (!classDecl.definitionReason) {
            return std::nullopt;
        }
        if (!expect("memory", "expected class memory declaration")) {
            return std::nullopt;
        }
        classDecl.memoryAccess = parseAccessFlags();
        if (!expect("inherits", "expected inherits clause")) {
            return std::nullopt;
        }
        auto base = expectIdentifier("expected base class name");
        if (!base) {
            return std::nullopt;
        }
        classDecl.baseClass = base->text;
        classDecl.inheritanceReason = parseBecauseLiteral("inheritance");
        if (!classDecl.inheritanceReason) {
            return std::nullopt;
        }
        if (!expect("patterns", "expected class design patterns clause")) {
            return std::nullopt;
        }
        while (!atEnd() && !check("injects") && !check("{")) {
            auto pattern = expectIdentifier("expected design pattern name");
            if (!pattern) {
                return std::nullopt;
            }
            classDecl.patterns.push_back(pattern->text);
        }
        if (!expect("injects", "expected injects dependency clause")) {
            return std::nullopt;
        }
        while (!atEnd() && !check("{")) {
            auto dependency = expectIdentifier("expected dependency name");
            if (!dependency) {
                return std::nullopt;
            }
            classDecl.dependencies.push_back(dependency->text);
        }
        if (!parseFieldBlock(classDecl, "class")) {
            return std::nullopt;
        }
        if (!parseImplementBlock(classDecl)) {
            return std::nullopt;
        }
        return classDecl;
    }

    bool parseFieldBlock(StructDecl& structure, std::string_view owner) {
        if (!expect("{", "expected { before fields")) {
            return false;
        }
        while (!atEnd() && !check("}")) {
            match("proceed");
            if (!check("declare")) {
                error(current(), "expected-field", std::string("expected field declaration in ") + std::string(owner) + " body");
                return false;
            }
            auto field = parseVarDecl();
            if (!field) {
                return false;
            }
            structure.fields.push_back(std::move(field->varDecl));
        }
        if (!expect("}", "expected } after struct fields")) {
            return false;
        }
        return true;
    }

    bool parseImplementBlock(StructDecl& structure) {
        if (!expect("implement", "expected implement block")) {
            return false;
        }
        if (!expect("{", "expected { before implement block")) {
            return false;
        }
        while (!atEnd() && !check("}")) {
            if (!check("method")) {
                error(current(), "expected-method", "expected method in implement block");
                return false;
            }
            auto method = parseFunction(true);
            if (!method) {
                return false;
            }
            structure.methods.push_back(std::move(*method));
        }
        expect("}", "expected } after implement block");
        return true;
    }

    bool skipBalancedBlock() {
        if (!expect("{", "expected {")) {
            return false;
        }
        int depth = 1;
        while (!atEnd() && depth > 0) {
            if (match("{")) {
                ++depth;
            } else if (match("}")) {
                --depth;
            } else {
                ++position_;
            }
        }
        return depth == 0;
    }

    bool validateEventBlockAndSkip() {
        if (!expect("{", "expected {")) {
            return false;
        }
        int depth = 1;
        while (!atEnd() && depth > 0) {
            if (current().text == "authorize" || current().text == "assign" || current().text == "operatorinput" ||
                current().text == "return" || current().text == "declare" || current().text == "function" ||
                current().text == "grant" || current().text == "delegate" || current().text == "revoke") {
                error(current(), "impure-event-block", "event blocks may only contain onread/onwrite/onchange gate expressions");
                return false;
            }
            if (match("{")) {
                ++depth;
            } else if (match("}")) {
                --depth;
            } else {
                ++position_;
            }
        }
        return depth == 0;
    }

    void synchronizeStatement() {
        while (!atEnd()) {
            if (previous().text == ";") {
                return;
            }
            if (current().text == "}") {
                return;
            }
            ++position_;
        }
    }

    std::span<const Token> tokens_;
    Diagnostics& diagnostics_;
    std::size_t position_ = 0;
};

} // namespace

std::optional<Program> parseTokens(std::span<const Token> tokens, Diagnostics& diagnostics) {
    if (tokens.empty()) {
        diagnostics.error("parser", "empty-token-stream", SourceLocation{"<tokens>", 1, 1}, "cannot parse an empty token stream");
        return std::nullopt;
    }
    Parser parser(tokens, diagnostics);
    return parser.parseProgram();
}

} // namespace torture::compiler
