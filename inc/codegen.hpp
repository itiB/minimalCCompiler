#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include "parser.hpp"

/// コード生成クラス
class CodeGen {
    llvm::LLVMContext context;
    // llvm::Module *module = new llvm::Module("top", context);
    // llvm::IRBuilder<> builder(context);
    private:
        llvm::Function    *CurFunc;   // 現在生成中のFunction
        llvm::Module      *Mod;       // 生成したModuleを格納する
        llvm::IRBuilder<> *Builder;   // LLVM_IRを生成するIRBuilderクラス

    public:
        CodeGen();
        ~CodeGen();
        bool doCodeGen(TranslationUnitAST &tunit, std::string name);
        llvm::Module &getModule();

    private:
        bool generateTranslationUnit(TranslationUnitAST &tunit, std::string name);
        llvm::Function *generateFunctionDefinition(FunctionAST *func, llvm::Module *mod);
        llvm::Function *generatePrototype(PrototypeAST *proto, llvm::Module *mod);
        llvm::Value *generateFunctionStatement(FunctionStmtAST *func_stmt);
        llvm::Value *generateVariableDeclaration(VariableDeclAST *vdecl);
        llvm::Value *generateStatement(BaseAST *stmt);
        llvm::Value *generateBinaryExpression(BinaryExprAST *bin_expr);
        llvm::Value *generateCallExpression(CallExprAST *call_expr);
        llvm::Value *generateJumpStatement(JumpStmtAST *jump_stmt);
        llvm::Value *generateVariable(VariableAST *var);
        llvm::Value *generateNumber(int value);
};