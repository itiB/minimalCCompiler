#include "ast.hpp"
#include "app.hpp"
#include "lexer.hpp"

#include<string>
#include<vector>
#include<map>
#include<algorithm>
#include<cstdio>
#include<cstdlib>

// S: 構文解析クラスの実装 p.80

// コンストラクタで入力ソースコード名を受け取る
// LexicalAnalysis関数でTokenStreamのインスタンスを生成

/// 構文解析、意味解析クラス
class Parser {
    private:
        TokenStream *Tokens;
        TranslationUnitAST *TU;

        //意味解析用各種識別子表
        // 解析中の関数の変数名を登録しておくベクタ
        std::vector<std::string> VariableTable;
        //
        std::map<std::string, int> PrototypeTable;
        //
        std::map<std::string, int> FunctionTable;

    public:
        Parser(std::string filename);
        ~Parser() {SAFE_DELETE(TU); SAFE_DELETE(Tokens);}

        // 構文解析開始トリガ
        // 外部から呼び出し、解析の成否を真偽値で返す
        bool doParse();

        // 解析結果ASTを外部から取得する
        // TranslationUnitASTはASTの頂点
        TranslationUnitAST &getAST();

    private:
        // 各解析メソッドの命名規則: visit<非終端記号名>
        // 返り値は基本的に解析して得られたASTクラス型のポインタ
        bool visitTranslationUnit();
        bool visitExternalDeclaration(TranslationUnitAST *tunit);
        PrototypeAST *visitFunctionDeclaration();
        FunctionAST *visitFunctionDefinition();
        PrototypeAST *visitPrototype();
        FunctionStmtAST *visitFunctionStatement(PrototypeAST *proto);
        VariableDeclAST *visitVariableDeclaration();
        BaseAST *visitStatement();
        BaseAST *visitExpressionStatement();
        BaseAST *visitJumpStatement();
        BaseAST *visitAssignmentExpression();
        BaseAST *visitAdditiveExpression(BaseAST *lhs);
        BaseAST *visitMultiplicativeExpression(BaseAST *lhs);
        BaseAST *visitPostfixExpression();
        BaseAST *visitPrimaryExpression();
};

// E: 構文解析クラスの実装 p.80
