// CodeGenクラスのメソッドを実装していく

#include "../inc/codegen.hpp"

/// コンストラクタ
/// IRBuilderを生成 各コード生成メソッドで使用する
/// IRBuilderのコンストラクタ: IRBuild(LLVMContext &c, MDNode *FPMathTag = 0)
/// - LLVMContext: LLVM Coreのデータを管理、提供するクラス
/// - MDNode: fpmath Metadata(ULP)を指定する、デフォルトは0
CodeGen::CodeGen() {
    // llvmContextはgetGlobalContext()でコンテキストが得られる
    Builder = new llvm::IRBuilder<>(context);
    Mod = NULL;
}

/// コード生成実行
/// @param TranslationUnitAST Module名(入力ファイル名)
/// @return 成功時: True, 失敗時: false
bool CodeGen::doCodeGen(TranslationUnitAST &tunit, std::string name) {
    return generateTranslationUnit(tunit, name);
}

/// モジュールの取得
/// doCodeGen()で作成されたModule(LLVM-IR)への参照を返す
/// Moduleがない、生成に失敗している場合は空のModuleを返す
llvm::Module &CodeGen::getModule() {
    if (Mod)
        return *Mod;
    else
        return *(new llvm::Module("null", context));
}

/// Module作成メソッド
/// Moduleを生成し、内包する関数のプロトタイプ宣言と関数定義の生成メソッドを呼ぶ
/// @param TranslationUnitAST Module名(入力ファイル名)
/// @return 成功時: True, 失敗時: false
bool CodeGen::generateTranslationUnit(TranslationUnitAST &tunit, std::string name) {
    // llvm::Moduleのコンストラクタ: Module(StringRef ModuleID, LLVMContext &C)
    // - ModuleID モジュールの名前
    // - LLVMContext IRBuilderと同じコンテキストを使えばOK
    Mod = new llvm::Module(name, context);

    // Function declaration
    for (int i = 0; ; i++) {
        PrototypeAST *proto = tunit.getPrototype(i);
        if (!proto) {
            break;
        } else if (!generatePrototype(proto, Mod)) {
            SAFE_DELETE(Mod);
            return false;
        }
    }

    // function definition
    for (int i = 0; ; i++) {
        FunctionAST *func = tunit.getFunction(i);
        if (!func) {
            break;
        } else if (!generateFunctionDefinition(func, Mod)) {
            SAFE_DELETE(Mod);
            return false;
        }
    }
    return true;
}



/// 関数宣言生成メソッド
/// PrototypeASTの情報からFunctionを生成し、Moduleに追加する
/// @param PrototypeAST, Module
/// @return 生成したFunctionのポインタ
llvm::Function *CodeGen::generatePrototype(PrototypeAST *proto, llvm::Module *mod) {
    // already declared??
    // getFunction(): 与えた名前のFunctionがModulu内に存在するならばFunctionを返す
    llvm::Function *func = mod->getFunction(proto->getName());
    if (func) {
        if (func->arg_size() == proto->getParamNum() && func->empty()) {
            return func;
        } else {
            fprintf(stderr, "error: function %s is redefined", proto->getName().c_str());
            return NULL;
        }
    }

    // create arg_type
    std::vector<llvm::Type*> int_types(proto->getParamNum(), llvm::Type::getInt32Ty(context));

    // create func type
    // - FunctionType: 戻り値や引数リストを表す
    //   llvm/DerivedTypes.h llvm::FunctionType::get
    //   static FunctionType * get(Type *Result, ArrayRef<Type *> Params, bool isVarArg)
    //   - Type: 戻り値の型
    //     llvm/Type.h llvm::Type::getInt32Ty
    //     static IntegerType * getInt32Ty(LLVMContext &C)
    //   - ArrayRef<Type *>: 引数の型を示したベクタ
    //   - isVarArg: 可変長かどうか
    llvm::FunctionType *func_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        int_types, false
    );

    // create function
    // Functionのボディは生成せずに空のプロトタイプ宣言だけのFunctionを生成
    // llvm/Function.h Function.Create()インタフェース
    // static Function * Create(FunctionType *Ty, LinkageTypes Linkage, const Twine &N="", Module *M=0)
    // - FunctionType: 上でつくったやつ
    // - LinkageType: GlobalValueクラスの列挙型
    // - Twine: 関数名の指定
    // - Module: 追加先Module
    func = llvm::Function::Create(func_type,
        llvm::Function::ExternalLinkage,
        proto->getName(), mod);
    
    // set names
    // Functionの引数イテレータをたどって引数名をPrototypeAST.getParamName() + "_arg"にする
    llvm::Function::arg_iterator arg_iter = func->arg_begin();
    for (int i = 0; i < proto->getParamNum(); i++) {
        arg_iter->setName(proto->getParamName(i).append("_arg"));
        arg_iter++;
    }

    return func;
}

/// 関数定義生成メソッド
/// 関数の生成、生成されたFunctionに対してBasicBlockの追加、generateFunctionStatementによるBodyの作成
/// @param FunctionAST Module
/// @return 生成したFunctionへのポインタ
llvm::Function *CodeGen::generateFunctionDefinition(FunctionAST * func_ast, llvm::Module *mod) {
    llvm::Function *func = generatePrototype(func_ast->getPrototype(), mod);
    if (!func) {
        return NULL;
    }
    CurFunc = func;

    // BasicBlockの作成: llvm/BasicBlock.h BasickBlock::Create
    // static BasicBlock * Create(LLVMContext &Context, const Twine &Name="", Function *Parent=0, BasicBlock *InsertBefore=0)
    // - Twine: BBにつける名前
    // - Function: BBを挿入するFunctionを指定
    // - InsertBefore: BBを挿入するFunction内の位置を指定
    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(context, "entry", func);

    // IRBuilderクラスのSetInsertPointメソッドで命令の挿入位置を指定
    // 引数の末尾に命令の挿入位置を設定
    // llvm::IRBuilderBase::SetInsertPoint
    // void SetInsertPoint(BasicBlock *TheBB)
    Builder->SetInsertPoint(bblock);

    // Functionのボディを作る
    generateFunctionStatement(func_ast->getBody());

    return func;
}

// 関数生成メソッド
/// 変数宣言、ステートメントの順に生成する
/// @param FunctionStmtAST
/// @return 最後に生成したValueのポインタ
llvm::Value *CodeGen::generateFunctionStatement(FunctionStmtAST *func_stmt) {
    // insert variable decls
    VariableDeclAST *vdecl;
    llvm::Value *v = NULL;

    // generateVariableDeclaration, generateStatementを用いてIRを作成する
    for (int i = 0; ; i++) {
        // 最後まで見たら終了する
        if (!func_stmt->getVariableDecl(i))
            break;

        // create alloca
        // generateVariableDeclaration: 引数で与えられたFunctionSTMTASTが保持する
        // VariableDeclASTと各種StatementのASTを探索、コード生成メソッドの呼び出し
        vdecl = llvm::dyn_cast<VariableDeclAST>(func_stmt->getVariableDecl(i));
        v = generateVariableDeclaration(vdecl);
    }

    // inser expr statement
    BaseAST *stmt;
    for (int i = 0; ; i++) {
        // 最後までみたら終了
        stmt = func_stmt->getStatement(i);
        if (!stmt)
            break;
        else if (!llvm::isa<NullExprAST>(stmt))
            v = generateStatement(stmt);
    }
    return v;
}

/// 変数宣言(alloca命令)生成メソッド
/// @param VariableDeclAST
/// @return 生成したValueへのポインタ
llvm::Value *CodeGen::generateVariableDeclaration(VariableDeclAST *vdecl) {
    // Alloca命令の作成: CreateAllocaを用いる
    // llvm::IRBuilder::CreateAlloca
    // AllocaInst * CreateAlloca(Type *ty, Value *ArraySize=0, const Twine &Name="")
    // - ty: 生成する変数の型を表すType
    // - value: 配列の長さを表す
    // - Name: 変数名の指定
    llvm::AllocaInst *alloca = Builder->CreateAlloca(
        llvm::Type::getInt32Ty(context),
        0,
        vdecl->getName()
    );

    // if args alloca
    // 関数内のほかの変数と同様に関数の引数に対してアクセスする
    // -> 引数とは別に関数内に引数を示す変数を作る

    // 引数Paramを表す宣言種別(DeclType)がgenerateVariableDeclarationに与えられた時、
    // CreateAllocaで確保した変数に引数の値を保存する(Store命令)
    // llvm::IRBuilder::CreateStore
    // StoreInst * CreateStore(Value *Val, Value *Ptr, bool isVolatile=false)
    // - Val: 格納する値
    // - Ptr: 格納先
    //   - Value: InstructionやGlobalValue, Functionの基底クラス
    //   - 引数を表すValueを取得する: FunctionのgetValueSymbolTable, ValueSymbolTableのloopupを利用
    if (vdecl->getType() == VariableDeclAST::param) {
        // Store Args p.119
        llvm::ValueSymbolTable &vs_table = CurFunc->getValueSymbolTable();
        Builder->CreateStore(vs_table.lookup(vdecl->getName().append("_arg")), alloca);
    }
    return alloca;
}

/// ステートメントの作成
/// 引数として与えられたBaseASTの種類を確認して各種生成メソッドを呼び出す
/// @param JumpStmtAST
/// @return 生成したValueのポインタ
llvm::Value *CodeGen::generateStatement(BaseAST *stmt) {
    if (llvm::isa<BinaryExprAST>(stmt)) {
        return generateBinaryExpression(llvm::dyn_cast<BinaryExprAST>(stmt));
    } else if (llvm::isa<CallExprAST>(stmt)) {
        return generateCallExpression(llvm::dyn_cast<CallExprAST>(stmt));
    } else if (llvm::isa<JumpStmtAST>(stmt)) {
        return generateJumpStatement(llvm::dyn_cast<JumpStmtAST>(stmt));
    } else {
        return NULL;
    }
}

/// 二項演算生成メソッド
/// @param JumpStmtAST
/// @return 生成したValueへのポインタ
llvm::Value *CodeGen::generateBinaryExpression(BinaryExprAST *bin_expr) {
    // 左辺値、右辺値のコードを生成
    BaseAST *lhs = bin_expr->getLHS();
    BaseAST *rhs = bin_expr->getRHS();

    llvm::Value *lhs_v;
    llvm::Value *rhs_v;

    // 代入文の生成
    if (bin_expr->getOp() == "=") {
        // lhs is variable
        VariableAST *lhs_var = llvm::dyn_cast<VariableAST>(lhs);
        llvm::ValueSymbolTable &vs_table = CurFunc->getValueSymbolTable();
        lhs_v = vs_table.lookup(lhs_var->getName());

    // other operand
    } else {
        // lhs = ?

        // Binary?
        if (llvm::isa<BinaryExprAST>(lhs)) {
            lhs_v = generateBinaryExpression(llvm::dyn_cast<BinaryExprAST>(lhs));
        
        // Variable?
        } else if (llvm::isa<VariableAST>(lhs)) {
            lhs_v = generateVariable(llvm::dyn_cast<VariableAST>(lhs));
        
        // Number?
        } else if (llvm::isa<NumberAST>(lhs)) {
            NumberAST *num = llvm::dyn_cast<NumberAST>(lhs);
            lhs_v = generateNumber(num->getNumberValue());
        }
    }

    // create rhs value
    if (llvm::isa<BinaryExprAST>(rhs)) {
        rhs_v = generateBinaryExpression(llvm::dyn_cast<BinaryExprAST>(rhs));

    // Variable?
    } else if (llvm::isa<VariableAST>(rhs)) {
        rhs_v = generateVariable(llvm::dyn_cast<VariableAST>(rhs));
    
    // Number?
    } else if (llvm::isa<NumberAST>(rhs)) {
        NumberAST *num = llvm::dyn_cast<NumberAST>(rhs);
        rhs_v = generateNumber(num->getNumberValue());
    }

    // 四則演算命令の生成
    if (bin_expr->getOp() == "=") {
        // store
        return Builder->CreateStore(rhs_v, lhs_v);
    } else if (bin_expr->getOp() == "+") {
        // add
        return Builder->CreateAdd(rhs_v, lhs_v, "add_tmp");
    } else if (bin_expr->getOp() == "-") {
        // sub
        return Builder->CreateSub(rhs_v, lhs_v, "sub_tmp");
    } else if (bin_expr->getOp() == "*") {
        // mul
        return Builder->CreateMul(rhs_v, lhs_v, "mul_tmp");
    } else if (bin_expr->getOp() == "/") {
        // div
        return Builder->CreateSDiv(rhs_v, lhs_v, "div_tmp");
    }
}

/// 関数呼び出し(call命令)生成メソッド
/// @param CallExprAST
/// @return 生成したValueのポインタ
llvm::Value *CodeGen::generateCallExpression(CallExprAST *call_expr) {
    std::vector<llvm::Value*> arg_vec;
    BaseAST *arg;
    llvm::Value *arg_v;
    llvm::ValueSymbolTable &vs_table = CurFunc->getValueSymbolTable();
    for (int i = 0; ; i++) {
        if (!(arg = call_expr->getArgs(i)))
            break;
        
        // is call
        if (llvm::isa<CallExprAST>(arg)) {
            arg_v = generateCallExpression(llvm::dyn_cast<CallExprAST>(arg));

        // isBinaryExpr
        } else if (llvm::isa<BinaryExprAST>(arg)) {
            BinaryExprAST *bin_expr = llvm::dyn_cast<BinaryExprAST>(arg);
            arg_v = generateBinaryExpression(llvm::dyn_cast<BinaryExprAST>(arg));
            if (bin_expr->getOp() == "=") {
                VariableAST *var = llvm::dyn_cast<VariableAST>(bin_expr->getLHS());
                arg_v = Builder->CreateLoad(vs_table.lookup(var->getName()), "arg_val");
            }

        // isVar
        } else if (llvm::isa<VariableAST>(arg)) {
            NumberAST *num = llvm::dyn_cast<NumberAST>(arg);
            arg_v = generateNumber(num->getNumberValue());
        }
        arg_vec.push_back(arg_v);
    }

    // LLVM::IRBuilder::CreateCall
    // CallInst * CreateCall(Value *Callee, ArrayRef<Value *> Args, const Twine &Name="")
    // - callee: 呼び出し対象Function, ModuleクラスにgetFunctionを関数名指定して取得する
    // - Args: 引数として渡すValue, std::vecrotに詰め込んで渡す
    // - name: 関数呼び出しの戻り値を角野数るレジスタ名
    return Builder->CreateCall(Mod->getFunction(call_expr->getCallee()), arg_vec, "call_tmp");
}

/// return文の生成
/// 渡されたJumpStmpAST(戻り値)を示すASTに応じたReturnを作る
/// @param JumpStmtAST
/// @return 生成したValueのポインタ
llvm::Value *CodeGen::generateJumpStatement(JumpStmtAST *jump_stmt) {
    BaseAST *expr = jump_stmt->getExpr();
    llvm::Value *ret_v;

    if (llvm::isa<BinaryExprAST>(expr)) {
        ret_v = generateBinaryExpression(llvm::dyn_cast<BinaryExprAST>(expr));
    } else if (llvm::isa<VariableAST>(expr)) {
        VariableAST *var = llvm::dyn_cast<VariableAST>(expr);
        ret_v = generateVariable(var);
    } else if (llvm::isa<NumberAST>(expr)) {
        NumberAST *num = llvm::dyn_cast<NumberAST>(expr);
        ret_v = generateNumber(num->getNumberValue());
    }
    // IRBuilder::CreateRef
    // ReturnInst * CreateRet(Value *V)
    Builder->CreateRet(ret_v);
}

/// 変数参照(load命令)生成メソッド p.124
/// @param VariableAST
/// @return 生成したValueのポインタ
llvm::Value *CodeGen::generateVariable(VariableAST *var) {
    llvm::ValueSymbolTable &vs_table = CurFunc->getValueSymbolTable();
    // llvm::IRBuilder::CreateLoad
    // LoadInst * CreateLoad(Value *Ptr, const Twine &Name="")
    // - Ptr: Load対象のValue
    //   - ValueSymbolTableからAllocaInstを取得して指定
    return Builder->CreateLoad(vs_table.lookup(var->getName()), "var_tmp");
}

/// 定数生成メソッド
/// 引数に与えられた値を示すValueを生成する
/// @param 生成する定数の値
/// @return 生成したValueのポインタ
llvm::Value *CodeGen::generateNumber(int value) {
    // llvm::ConstantInt::get
    // static ConstantInt * get(LLVMContext &Context, const APInt &V)
    // - V: 整数値、int型を与える
    return llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context),
        value
    );
}
