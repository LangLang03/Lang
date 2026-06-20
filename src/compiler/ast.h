#pragma once

#include "common/diagnostic.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace torture::compiler {

class AstVisitor;

struct TypeName {
    std::string text;
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct FunctionCallArg {
    std::string name;
    bool byReference = false;
    ExprPtr value;
    SourceLocation location;
};

struct IoClause {
    bool present = false;
    ExprPtr reason;
    SourceLocation location;
};

struct AuthorizeCall {
    std::string target;
    std::vector<FunctionCallArg> arguments;
    bool discardingReturn = false;
    bool viaPointer = false;
    bool hasNullCheck = false;
    bool nullCheck = false;
    int securityLevel = 0;
    int memoryLimit = 0;
    int timeout = 0;
    int predictStackDepth = 0;
    int approvalTimeout = 0;
    std::string authorityChain;
    std::optional<std::string> approvalFunction;
    IoClause io;
    SourceLocation location;
};

struct GateOp {
    std::string op;
    std::string left;
    std::string right;
    std::string output;
    SourceLocation location;
};

enum class ExprKind {
    Integer,
    Boolean,
    String,
    Null,
    Identifier,
    Unary,
    Binary,
    Conditional,
    Compute,
    Field,
    Gate,
    AuthorizeCall,
};

struct Expr {
    ExprKind kind = ExprKind::Null;
    SourceLocation location;
    std::string text;
    bool declaredLiteral = false;
    bool operatorAuthorized = false;
    ExprPtr left;
    ExprPtr right;
    ExprPtr third;
    ExprPtr operatorReason;
    std::vector<GateOp> gateOps;
    AuthorizeCall authorizeCall;

    void accept(AstVisitor& visitor) const;
};

ExprPtr makeExpr(ExprKind kind, SourceLocation location, std::string text = {});
ExprPtr cloneExpr(const Expr& expr);

struct VarDecl {
    bool immutable = false;
    std::string access;
    std::string purpose;
    std::string scope;
    bool isVolatile = false;
    TypeName type;
    std::string name;
    ExprPtr initializer;
    SourceLocation location;

    void accept(AstVisitor& visitor) const;
};

struct AssignStmt {
    ExprPtr target;
    ExprPtr value;
    SourceLocation location;
};

struct FptrAssignStmt {
    std::string variable;
    std::string target;
    std::string authorityChain;
    SourceLocation location;
};

struct ProofStmt {
    std::string form;
    std::string name;
    ExprPtr premise;
    ExprPtr claim;
    ExprPtr reason;
    SourceLocation location;
};

struct FormalClause {
    std::string kind;
    ExprPtr expression;
    ExprPtr reason;
    SourceLocation location;
};

struct JudgmentClause {
    bool present = false;
    std::string authority;
    ExprPtr reason;
    int declaredElseIfCount = -1;
    int declaredElseCount = -1;
    SourceLocation location;
};

struct RoleStmt {
    std::string op;
    std::string role;
    std::string principal;
    std::string chain;
    SourceLocation location;
};

struct Statement;
using StatementPtr = std::unique_ptr<Statement>;

enum class StatementKind {
    VarDecl,
    Assign,
    AssignFromCall,
    Return,
    VerifyFunctionIdentity,
    AuthorizeCall,
    FptrAssign,
    Proof,
    Gate,
    OperatorInput,
    Role,
    If,
    While,
    DoUntil,
    For,
    Break,
    Continue,
    ClockCycle,
    YieldGate,
    Instantiate,
    Unknown,
};

struct InstantiateStmt {
    std::string className;
    std::string name;
    std::string authority;
    std::string memoryAccess;
    std::string factory;
    std::string strategy;
    std::string dependency;
    ExprPtr reason;
    SourceLocation location;
};

struct Statement {
    StatementKind kind = StatementKind::Unknown;
    SourceLocation location;
    bool proceeded = false;
    VarDecl varDecl;
    AssignStmt assign;
    ExprPtr expression;
    ExprPtr initializer;
    ExprPtr increment;
    AuthorizeCall authorizeCall;
    IoClause io;
    FptrAssignStmt fptrAssign;
    ProofStmt proof;
    RoleStmt role;
    InstantiateStmt instantiate;
    std::string identifier;
    JudgmentClause judgment;
    JudgmentClause elseJudgment;
    std::vector<FormalClause> formalClauses;
    std::vector<StatementPtr> thenBody;
    std::vector<StatementPtr> elseBody;

    void accept(AstVisitor& visitor) const;
};

StatementPtr makeStatement(StatementKind kind, SourceLocation location);

struct Param {
    std::string access;
    TypeName type;
    std::string name;
    SourceLocation location;
};

enum class FunctionCategory {
    Callable,
    Uncallable,
    Approval,
};

struct FunctionDecl {
    std::string name;
    std::string access;
    FunctionCategory category = FunctionCategory::Callable;
    bool returnable = false;
    TypeName returnType;
    std::vector<Param> params;
    int codeSize = 0;
    int maxStackSize = 0;
    int securityLevel = 0;
    std::string grantor;
    int declaredLocalCount = -1;
    std::string definitionAuthority;
    std::vector<std::string> callableFrom;
    std::vector<std::string> allowedRoles;
    std::vector<StatementPtr> body;
    SourceLocation location;

    void accept(AstVisitor& visitor) const;
};

struct StructDecl {
    bool isClass = false;
    std::string name;
    int declaredFieldCount = -1;
    int declaredMethodCount = -1;
    std::string definitionAuthority;
    ExprPtr definitionReason;
    std::string memoryAccess;
    std::string baseClass;
    ExprPtr inheritanceReason;
    std::vector<std::string> patterns;
    std::vector<std::string> dependencies;
    std::vector<VarDecl> fields;
    std::vector<FunctionDecl> methods;
    SourceLocation location;

    void accept(AstVisitor& visitor) const;
};

struct Program {
    bool requireEcc = false;
    std::vector<FunctionDecl> functions;
    std::vector<StructDecl> structs;

    void accept(AstVisitor& visitor) const;
};

std::string summarizeProgram(const Program& program);

} // namespace torture::compiler
