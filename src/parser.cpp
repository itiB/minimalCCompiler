#include "../inc/parser.hpp"

// S: 構文解析メソッドの実装 p.81

/// コンストラクタ
Parser::Parser(std::string filename) {
    // TokenStreamクラスのインスタンスをTokensに保存する
    Tokens = LexicalAnalysis(filename);
}

/// 構文解析実行
/// @return 解析成功: true, 解析失敗: false
bool Parser::doParse() {
    if(!Tokens) {
        fprintf(stderr, "error at lexer\n");
        return false;
    } else {
        // 構文解析
        return visitTranslationUnit();
    }
}

/// ASTの取得
/// @return TranslationUnitへの参照
TranslationUnitAST &Parser::getAST() {
    if (TU) {
        return *TU;
    } else {
        return *(new TranslationUnitAST());
    }
}

// E: 構文解析メソッドの実装 p.81

/// TranslationUnit用構文解析メソッド
/// @return 解析成功: true, 解析失敗: false
/// -+-> external_declaration -+->
///  ^                         |
///  └-------------------------┘
bool Parser::visitTranslationUnit() {
    TU = new TranslationUnitAST();

    // ExternalDecl
    while (true) {
        if (!visitExternalDeclaration(TU)) {
            SAFE_DELETE(TU);
            return false;
        } 
        if (Tokens->getCurType() == TOK_EOF) {
            break;
        }
    }
    return true;
}

/// ExternalDeclaration用構文解析クラス
/// 解析したPrototypeとFunctionASTをTranslationUnitに追加
/// @param TranslationUnitAST
/// @return true
/// -+-> function_declaration-+->
///  |                        ^
///  └-> function_definition -┘
bool Parser::visitExternalDeclaration(TranslationUnitAST *tunit) {
    // FunctionDeclaration
    PrototypeAST *proto = visitFunctionDeclaration();
    if (proto) {
        tunit->addPrototype(proto);
        return true;
    }

    // FunctionDrfinition
    FunctionAST *func_def = visitFunctionDefinition();
    if (func_def) {
        tunit->addFunction(func_def);
        return true;
    }
    return false;
}

/// Prototype用構文解析メソッド
/// @return 解析成功: PrototypeAST 解析失敗: NULL
PrototypeAST *Parser::visitFunctionDeclaration() {
    int bkup = Tokens->getCurIndex();
    PrototypeAST *proto = visitPrototype();
    if (!proto) {
        return NULL;
    }

    // prototype
    if (Tokens->getCurString() == ";") {
        // TODO: 再定義されていないか確認する処理

        Tokens->getNextToken();
        return proto;
    } else {
        SAFE_DELETE(proto);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

/// FunctionDefinition用構文解析メソッド
/// @return 解析成功: FunctionAST 解析失敗: NULL
FunctionAST *Parser::visitFunctionDefinition() {
    int bkup = Tokens->getCurIndex();

    PrototypeAST *proto = visitPrototype();
    if(!proto) {
        return NULL;
    }

    // TODO: 再定義されていないか確認

    FunctionStmtAST *func_stmt = visitFunctionStatement(proto);
    if (func_stmt) {
        FunctionTable[proto->getName()] = proto->getParamNum();
        return new FunctionAST(proto, func_stmt);
    } else {
        SAFE_DELETE(proto);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

/// Prototype用構文解析メソッド
/// @return 解析成功: FunctionAST 解析失敗: NULL
///->int->identifier->( -+-> parameter -+-> ) ->
///                            ^----- , <-----┘
PrototypeAST *Parser::visitPrototype() {
    // bkup index
    int bkup = Tokens->getCurIndex();

    // todo 省略 p86

    // parameter_list
    bool is_first_param = true;
    std::vector<std::string> param_list;
    while (true)
    {
        // ,
        if (!is_first_param && Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ",") {
            Tokens->getNextToken();
        }
        if (Tokens->getCurType() == TOK_INT) {
            Tokens->getNextToken();
        } else {
            break;
        }

        if (Tokens->getCurType() == TOK_IDENTIFIER) {
            // TODO 引数名に重複がないかを確認

            param_list.push_back(Tokens->getCurString());
            Tokens->getNextToken();
        } else {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    // 省略 p87
}

// visitFunctionStatement p88

/// FunctionStatement用解析メソッド
/// @param 関数名や引数を格納したPrototypeクラスのインスタンス
/// @return 解析成功: FunctionStmtAST, 解析失敗: NULL
/// Function_statement: 変数宣言のリストと式のリストを含む関数ブロックを表す非終端記号
/// functionStatementを受け入れるために visitFunctionStatement
FunctionStmtAST *Parser::visitFunctionStatement(PrototypeAST *proto) {
    int bkup = Tokens->getCurIndex();

    if (Tokens->getCurString() == "{") {
        Tokens->getNextToken();
    } else {
        return NULL;
    }

    FunctionStmtAST *func_stmt = new FunctionStmtAST();

    // 引数をfunc_stmtの変数宣言リストに追加
    for (int i = 0; i < proto->getParamNum(); i++) {
        VariableDeclAST *vdecl = new VariableDeclAST(proto->getParamName(i));
        vdecl->setDeclType(VariableDeclAST::param);
        func_stmt->addVariableDeclaration(vdecl);
        // VariableTable.push_back(vdecl->getName());
    }

    VariableDeclAST *var_decl;
    BaseAST *stmt;
    BaseAST *last_stmt;

    // {statement_list}
    if (stmt = visitStatement()){
        while (stmt) {
            last_stmt = stmt;
            func_stmt->addStatement(stmt);
            stmt = visitStatement();
        }

    // variable_declaration_list
    } else if (var_decl = visitVariableDeclaration()) {
        while (var_decl){
            var_decl->setDeclType(VariableDeclAST::local);

            // 変数の2重宣言チェック
            if(std::find(VariableTable.begin(), VariableTable.end(), var_decl->getName()) != VariableTable.end()){
                SAFE_DELETE(var_decl);
                SAFE_DELETE(func_stmt);
                return NULL;
            }

            func_stmt->addVariableDeclaration(var_decl);
            VariableTable.push_back(var_decl->getName());
            // parse Variable Delaration
            var_decl=visitVariableDeclaration();
        }

        if (stmt = visitStatement()) {
            while (stmt) {
                last_stmt = stmt;
                func_stmt->addStatement(stmt);
                stmt = visitStatement();
            }
        }

        // other
    } else {
        SAFE_DELETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }

    // TODO: 戻り値の確認

    if (Tokens->getCurString() == "}") {
        Tokens->getNextToken();
        return func_stmt;
    } else {
        SAFE_DELETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

/// AssignmentExpression(代入文)用構文解析メソッド
/// 非終端記号assignment_expressionの解析
/// @return 解析成功: AST, 解析失敗: NULL
/// -+-> identifier -> = -> additive_expression -+->
///  |                                           ^
///  └-> additive_exprssion----------------------┘
BaseAST *Parser::visitAssignmentExpression() {
    int bkup = Tokens->getCurIndex();

    BaseAST *lhs;
    if (Tokens->getCurType() == TOK_IDENTIFIER) {
        // TODO: 変数宣言の確認 p.90

        // 左辺値: 識別子(変数名)
        lhs = new VariableTable(Tokens->getCurString());

        Tokens->getNextToken();
        BaseAST *rhs;
        if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "=") {
            Tokens->getNextToken();
            if (rhs = visitAdditiveExpression(NULL)) {
                return new BinaryExprAST("=", lhs, rhs);
            } else {
                SAFE_DELETE(lhs);
                Tokens->applyTokenIndex(bkup);
            }
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
        }
    }
    BaseAST *add_expr = visitAdditiveExpression(NULL);
    if (add_expr) {
        return add_expr;
    }
    return NULL;
}

/// Primary_expression(式の基本構成要素)用構文解析メソッド
/// 識別子や数値、()で囲まれた式を処理する
/// @return 解析成功時: AST, 解析失敗: NULL
BaseAST *Parser::visitPrimaryExpression() {
    // record index
    int bkup = Tokens->getCurIndex();

    // VARIABLE_IDENTIFIER
    if (Tokens->getCurType() == TOK_IDENTIFIER) {
        // TODO: 変数宣言の確認 p91

        std::string var_name = Tokens->getCurString();
        Tokens->getNextToken();
        return new VariableAST(var_name);

    // integer
    } else if (Tokens->getCurType() == TOK_DIGIT) {
        int val = Tokens->getCurNumVal();
        Tokens->getNextToken();
        return new NumberAST(val);
    
    // integer(-)
    } else if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "-") {
        // TODO: 負数の処理 p92
    }
    // TODO: '(', ')' expression

    // 何にも当てはまらなければNULL
    return NULL;
}

/// PostfixExpression(関数呼び出し)用構文解析メソッド
/// @return 解析成功: AST, 解析失敗: NULL
/// -+-> primary_expression ---------------------------+->
///  |                                                 ^
///  └-> identifier -> ( -+-> assignment_expression -+-┘
///                       ^                          |
///                       └------- , --------------- ┘
BaseAST *Parser::visitPostfixExpression() {
    // get index
    int bkup = Tokens->getCurIndex();

    // primary_expression
    BaseAST *prim_expr = visitPrimaryExpression();
    if (prim_expr){
        return prim_expr;
    }

    // FUNCTION_IDENTIFIER
    if (Tokens->getCurType() == TOK_IDENTIFIER) {
        // TODO: 関数宣言の確認

        // 関数名取得
        std::string Callee = Tokens->getCurString();
        Tokens->getNextToken();

        // LEFT PARENの存在確認
        if (Tokens->getCurType() != TOK_SYMBOL || Tokens->getCurString() != "(") {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

        Tokens->getNextToken();
        std::vector<BaseAST*> args;

        // 引数の解析
        BaseAST *assign_expr = visitAssignmentExpression();
        if (assign_expr) {
            args.push_back(assign_expr);

            // "," が続く間続ける
            while (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ",") {
                Tokens->getNextToken();
                assign_expr = visitAssignmentExpression();

                if (assign_expr) {
                    args.push_back(assign_expr);
                } else {
                    break;
                }
            }
        }
        
        // Todo: 引数の数を確認する

        // RIGHT PALENの確認
        if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ")") {
            Tokens->getNextToken();
            return new CallExprAST(Callee, args);
        } else {
            // 復帰処理
            for (int i = 0; i < args.size(); i++) {
                SAFE_DELETE(args[i]);
            }
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    } else {
        return NULL;
    }
}

/// Additive_expression(加減算)用構文解析メソッド
/// @param lhs(左辺) 初回呼び出しはNULL
/// @return 解析成功: AST、解析失敗: NULL
/// -> multiplicative_expression -+-------------------------+->
///                               |                         ^
///         +---------------------+                         |
///         |                                               |
///         +---+--+-> + +> multiplicative_expression +-+-+-+
///             ^  |                                    ^ |
///             |  +-> - +> multiplicative_expression +-+ |
///             |                                         |
///             +-----------------------------------------+
BaseAST *Parser::visitAdditiveExpression(BaseAST *lhs) {
    int bkup = Tokens->getCurIndex();

    // 左辺値の取得
    if (!lhs) {
        lhs = visitMultiplicativeExpression(NULL);
    }

    if (!lhs) {
        return NULL;
    }

    BaseAST *rhs;
    // + 演算子の取得
    if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "+") {
        Tokens->getNextToken();
        // 右辺値の取得
        rhs = visitMultiplicativeExpression(NULL);
        if (rhs) {
            // 再帰的に呼び出して後続の演算も見る
            return visitAdditiveExpression(
                new BinaryExprAST("+", lhs, rhs)
            );
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

    // - 演算子の取得
    } else if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "-") {
        Tokens->getNextToken();
        // 右辺値の取得
        rhs = visitMultiplicativeExpression(NULL);
        if (rhs) {
            return visitAdditiveExpression(
                new BinaryExprAST("+", lhs, rhs)
            );
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    return lhs;
}

/// ExpressionStatement(;のみの分)用構文解析メソッド
/// @return 解析成功: AST, 解析失敗: NULL
BaseAST *Parser::visitExpressionStatement() {
    BaseAST *assign_expr;

    // NULL Expression
    if (Tokens->getCurString() == ";") {
        Tokens->getNextToken();
        return new NullExprAST();
    } else if (assign_expr = visitAssignmentExpression()) {
        if (Tokens->getCurString() == ";") {
            Tokens->getNextToken();
            return assign_expr;
        }
    }
    return NULL;
}
