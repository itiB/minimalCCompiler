#ifndef PARSER_HPP
#define PARSER_HPP

#include "parser.hpp"

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
        // 再定義されていないか確認する処理
        // 関数がすでに宣言されているかどうか
        // または関数が定義済み、引数の数があっているかを確認する
        if (PrototypeTable.find(proto->getName()) != PrototypeTable.end() ||
           (FunctionTable.find(proto->getName()) != FunctionTable.end() &&
           FunctionTable[proto->getName()] != proto->getParamNum())) {
            // 再定義されているならばエラーメッセージを出してNULLを返す
            fprintf(stderr, "Function: %s is redefined", proto->getName().c_str());
            SAFE_DELETE(proto);
            return NULL;
        }
        // (関数名, 引数)のペアをプロトタイプ宣言テーブル(Map)に追加
        PrototypeTable[proto->getName()] = proto->getParamNum();
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

    // プロトタイプ宣言と違いがないか
    // すでに関数定義が行われていないかを確認
    } else if ( (PrototypeTable.find(proto->getName()) != PrototypeTable.end() &&
      PrototypeTable[proto->getName()] != proto->getParamNum() ) ||
      FunctionTable.find(proto->getName()) != FunctionTable.end() ) {
        // エラーメッセージを出してNULLを返す
        fprintf(stderr, "Function: %s is redefined", proto->getName().c_str());
        SAFE_DELETE(proto);
        return NULL;
    }

    // 関数ごとに宣言済み変数を登録する
    // FunctionStatementの解析前に毎回クリアする
    VariableTable.clear();
    FunctionStmtAST *func_stmt = visitFunctionStatement(proto);
    if (func_stmt) {
        // (関数名, 引数の数)のペアを関数テーブル(Map)に追加
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
///                      ^----- , <-----┘
PrototypeAST *Parser::visitPrototype() {
    // bkup index
    int bkup = Tokens->getCurIndex();
    std::string func_name;

    // プロトタイプ宣言の詳細をとってくる
    //type_specifier
    if(Tokens->getCurType()==TOK_INT){
        Tokens->getNextToken();
    }else{
        return NULL;
    }

    //IDENTIFIER
    if(Tokens->getCurType()==TOK_IDENTIFIER){
        func_name=Tokens->getCurString();
        Tokens->getNextToken();
    }else{
        Tokens->ungetToken(1);  //unget TOK_INT
        return NULL;
    }

    //'('
    if(Tokens->getCurString()=="("){
        Tokens->getNextToken();
    }else{
        Tokens->ungetToken(2);	//unget TOK_INT IDENTIFIER
        return NULL;
    }

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
            // 引数名に重複がないかを確認
            // param_list(引数名を格納したリスト)に読み取った識別子が存在するか
            if (std::find(param_list.begin(), param_list.end(), Tokens->getCurString()) != param_list.end()) {
                Tokens->applyTokenIndex(bkup);
                return NULL;
            }
            // 存在しなければリストに識別子を追加する
            param_list.push_back(Tokens->getCurString());
            Tokens->getNextToken();
        } else {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }

    //')'
    if(Tokens->getCurString()==")"){
        Tokens->getNextToken();
        return new PrototypeAST(func_name, param_list);
    }else{
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

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
            // 変数名テーブルに新しく読み取った変数名を追加
            VariableTable.push_back(var_decl->getName());
            func_stmt->addVariableDeclaration(var_decl);
            // parse Variable Delaration
            var_decl = visitVariableDeclaration();
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

    // 戻り値の確認
    // 最後のstatementがjumpstatementであるかを確認
    if (!last_stmt || !llvm::isa<JumpStmtAST>(last_stmt)) {
        SAFE_DELETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }

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
        // 変数宣言の確認
        // 左辺の代入される変数は宣言済みの変数であることを確認
        if (std::find(VariableTable.begin(), VariableTable.end(), Tokens->getCurString()) != VariableTable.end()) {
            // 左辺値: 識別子(変数名)
            lhs = new VariableAST(Tokens->getCurString());
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
        } else {
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

    // 変数が宣言されていることを確認
    // VARIABLE_IDENTIFIER
    if (Tokens->getCurType() == TOK_IDENTIFIER &&
        std::find(VariableTable.begin(), VariableTable.end(), Tokens->getCurString()) != VariableTable.end()) {
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
        int param_num;
        // 関数宣言の確認
        // プロトタイプ宣言されているか確認し、引数の数をテーブルから取得
        if (PrototypeTable.find(Tokens->getCurString()) != PrototypeTable.end()) {
            param_num = PrototypeTable[Tokens->getCurString()];
        } else if (FunctionTable.find(Tokens->getCurString()) != FunctionTable.end()) {
            param_num = FunctionTable[Tokens->getCurString()];
        } else {
            return NULL;
        }

        // 関数名取得
        std::string Callee = Tokens->getCurString();
        Tokens->getNextToken();

        // LEFT PARENの存在確認
        if (Tokens->getCurType() != TOK_SYMBOL || Tokens->getCurString() != "(") {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

        Tokens->getNextToken();
        // 解析に成功した引数を格納するベクタ
        std::vector<BaseAST*> args;

        // 引数の解析
        BaseAST *assign_expr = visitAssignmentExpression();
        if (assign_expr) {
            args.push_back(assign_expr);

            // "," が続く間続ける
            while (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ",") {
                Tokens->getNextToken();

                // IDENTIFIER
                assign_expr = visitAssignmentExpression();
                if (assign_expr) {
                    args.push_back(assign_expr);
                } else {
                    break;
                }
            }
        }

        // 引数の数を確認する
        if (args.size() != param_num) {
            for (int i = 0; i < args.size(); i++) {
                SAFE_DELETE(args[i]);
            }
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

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

/// MultiplicativeExpression用解析メソッド
/// @param lhs(左辺) 初回呼び出しはNULL
/// @return 解析成功: AST, 解析失敗: NULL
BaseAST *Parser::visitMultiplicativeExpression(BaseAST *lhs) {
    int bkup = Tokens->getCurIndex();

    // BaseAST *lhs = visitPostfixExpression();
    if (!lhs) {
        lhs = visitPostfixExpression();
    }
    BaseAST *rhs;

    if (!lhs) {
        return NULL;
    }

    // *
    if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "*") {
        Tokens->getNextToken();
        rhs = visitPostfixExpression();

        if (rhs) {
            return visitMultiplicativeExpression(new BinaryExprAST("*", lhs, rhs));
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

    // /
    } else if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "/") {
        Tokens->getNextToken();
        rhs = visitPostfixExpression();

        if (rhs) {
            return visitMultiplicativeExpression(new BinaryExprAST("/", lhs, rhs));
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    return lhs;
}

// なんか実装し忘れてたメソッドたちを追加していく

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

BaseAST *Parser::visitStatement() {
    BaseAST *stmt = NULL;
    if (stmt = visitExpressionStatement()) {
        return stmt;
    } else if (stmt = visitJumpStatement()) {
        return stmt;
    } else {
        return NULL;
    }
}

/// VariableDeclaration用構文解析メソッド
/// @return 解析成功: VariableDeclAST, 解析失敗: NULL
VariableDeclAST *Parser::visitVariableDeclaration() {
    std::string name;

    // INT
    if (Tokens->getCurType() == TOK_INT) {
        Tokens->getNextToken();
    } else {
        return NULL;
    }

    // IDENTIFIER
    if (Tokens->getCurType() == TOK_IDENTIFIER) {
        name = Tokens->getCurString();
        Tokens->getNextToken();
    } else {
        Tokens->ungetToken(1);
        return NULL;
    }

    // ';'
    if (Tokens->getCurString() == ";") {
        Tokens->getNextToken();
        return new VariableDeclAST(name);
    } else {
        Tokens->ungetToken(2);
        return NULL;
    }
}

/// JumpStatement用構文解析メソッド
/// @return 解析成功: AST 解析失敗: NULL
BaseAST *Parser::visitJumpStatement() {
    int bkup = Tokens->getCurIndex();
    BaseAST *expr;

    if (Tokens->getCurType() == TOK_RETURN) {
        Tokens->getNextToken();
        if (!(expr = visitAssignmentExpression())) {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

        if (Tokens->getCurString() == ";") {
            Tokens->getNextToken();
            return new JumpStmtAST(expr);
        } else {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    } else {
        return NULL;
    }
}

#endif