#include<string>
#include "../inc/app.hpp"
#include<vector>

/// ｸﾗｽ宣言
class BaseAST;
class VariableAST;
class NumberAST;
class VariableDeclAST;
class BinaryExprAST;
class CallExprAST;
class JumpStmtAST;
class TranslationUnitAST;
class PrototypeAST;
class FunctionAST;
class FunctionStmtAST;

/// ASTの種類
enum AstID {
    BaseID,
    VariableID,
    NumberID,
    VariableDeclID,
    BinaryExprID,
    CallExprID,
    JumpStmtID,
    NullExprID,
};

// S: ステートメントとエクスプレッションの定義 p69

/// ASTの基底ｸﾗｽ
class BaseAST {
    AstID ID;

    public:
    BaseAST(AstID id):ID(id) {}
    virtual ~BaseAST() {}
    AstID getValueID() const {
        return ID;
    }
};

/// ";"を表すAST p96
class NullExprAST: public BaseAST {
    public:
        NullExprAST(): BaseAST(NullExprID) {}
        static inline bool classof(NullExprAST const*) {return true;}
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == NullExprID;
        }
};

/// 変数参照を表すｸﾗｽ
class VariableAST: public BaseAST {
    // 変数名(ﾒﾝﾊﾞ変数)
    std::string Name;

    public:
        VariableAST(const std::string &name) : BaseAST(VariableID), Name(name) {}
        ~VariableAST() {}

        // VariableASTなのでtrueを返す
        static inline bool classof(VariableAST const*) { return true; }

        // 渡されたBaseASTがVariableASTかを判定する
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == VariableID;
        }

        // 変数名の取得
        std::string getName() { return Name; }
};

/// 整数型を表すAST
class NumberAST: public BaseAST {
    // 数値情報
    int Val;
    
    public:
        NumberAST(int val) : BaseAST(NumberID), Val(val){}
        ~NumberAST() {}

        // NumberASTなのでtrueを返す
        static inline bool classof(NumberAST const*) { return true; }

        // 渡されたBaseASTがNumberASTか判別する
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == NumberID;
        }

        // このASTが表現する数値を取得する
        int getNumberValue() { return Val; }
};

/// 変数宣言を表すAST
/// 予約語 `int` で定義される
class VariableDeclAST: public BaseAST {
    public:
        typedef enum {
            param,
            local
        } DeclType;
    
    private:
        // 変数名
        std::string Name;
        // 変数宣言の種類
        DeclType Type;

    public:
        VariableDeclAST(const std::string &name) : BaseAST(VariableDeclID), Name(name) {}

        // VariableDeclASTなのでtrue
        static inline bool classof(VariableDeclAST const*) { return true; }

        // 渡されたBaseAstｸﾗｽがVariableDeclASTか判定
        static inline bool classof(BaseAST const* base) {
            return base -> getValueID() == VariableDeclID;
        }
        ~VariableDeclAST() {}

        // 変数名の取得
        std::string getName() { return Name; }

        // 変数の宣言種別を設定
        bool setDeclType(DeclType type) { Type = type; return true; }

        // 変数の宣下種別を設定
        DeclType getType() { return Type; }
};

/// 二項演算子を表すAST
class BinaryExprAST: public BaseAST {
    // 演算子の文字列表現
    std::string Op;
    // 二項演算子の左辺と右辺
    BaseAST *LHS, *RHS;

    public:
        BinaryExprAST(std::string op, BaseAST *lhs, BaseAST *rhs) : BaseAST(BinaryExprID), Op(op), LHS(lhs), RHS(rhs) {}
        ~BinaryExprAST() {SAFE_DELETE(LHS); SAFE_DELETE(RHS);}

        // BinaryExprASTなのでtrue
        static inline bool classof(BinaryExprAST const*) { return true; }

        // 渡されたBaseASTがBinaryExprASTか判定
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == BinaryExprID;
        }

        // 演算子を取得する
        std::string getOp() { return Op; }

        // 左辺値を取得
        BaseAST *getLHS() { return LHS; }

        // 右辺を取得
        BaseAST *getRHS() { return RHS; }
};

/// 関数呼び出しを表すAST
class CallExprAST: public BaseAST {
    // 関数名
    std::string Callee;
    // 関数呼び出しの引数
    std::vector<BaseAST*> Args;
    
    public:
        CallExprAST(const std::string &callee, std::vector<BaseAST*> &args) : BaseAST(CallExprID), Callee(callee), Args(args) {}
        ~CallExprAST();

        // callASTなのでTrue
        static inline bool classof(CallExprAST const*) { return true; }

        // 渡されたBaseASTがCallExprASTか判別する
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == CallExprID;
        }

        // 呼び出す関数名の取得
        std::string getCallee() { return Callee; }

        // i番目の引数を取得
        BaseAST *getArgs(int i) {
            if (i < Args.size()) return Args.at(i); else return NULL;
        }
};

/// ｼﾞｬﾝﾌﾟ(return)を表すAST
/// return と 二項演算で定義される
class JumpStmtAST: public BaseAST {
    // 戻り値となるAST
    BaseAST *Expr;
    public:
        JumpStmtAST(BaseAST *expr): BaseAST(JumpStmtID), Expr(expr) {}
        ~JumpStmtAST() { SAFE_DELETE(Expr); }

        // JumpStmtASTなのでtrueを返す
        static inline bool classof(JumpStmtAST const*) { return true; }

        // 渡されたBaseASTがJumpStmtASTか判定する
        static inline bool classof(BaseAST const* base) {
            return base->getValueID() == JumpStmtID;
        }

        // return で返すExpressionを取得する
        BaseAST *getExpr() { return Expr; }
};

// E: ステートメントとエクスプレッションの定義 p69

// S: 関数とモジュール p76

/// 関数定義(ステートメントの集合体)を表すAST
/// statement, valiable_declarationのリスト -> function_statement
class FunctionStmtAST {
    std::vector<VariableDeclAST*> VariableDecls;
    std::vector<BaseAST*> StmtLists;

    public:
        FunctionStmtAST() {}~FunctionStmtAST();

        // 関数に変数を追加する
        bool addVariableDeclaration(VariableDeclAST *vdecl);

        // 関数にステートメントを追加する
        bool addStatement(BaseAST *stmt) { StmtLists.push_back(stmt); }

        // i番目変数を取得する
        VariableDeclAST *getVariableDecl(int i) {
            if (i < VariableDecls.size()) {
                return VariableDecls.at(i);
            } else {
                return NULL;
            }
        }

        // i番目のステートメントを取得する
        BaseAST *getStatement(int i) {
            if (i < StmtLists.size()) {
                return StmtLists.at(i);
            } else {
                return NULL;
            }
        }
};

/// プロトタイプ宣言の情報を保存するためのAST
/// 変数名(引数)リストと関数を持つクラスとして定義
class PrototypeAST {
    // 変数名
    std::string Name;
    // 引数の変数名
    std::vector<std::string> Params;

    public:
        PrototypeAST(const std::string &name, const std::vector<std::string> &params) : Name(name), Params(params){}

        // 関数名を取得する
        std::string getName() { return Name; }

        // i番目の引数名を取得する
        std::string getParamName(int i) {
            if (i < Params.size()) {
                return Params.at(i);
            } else {
                return NULL;
            }
        }

        // 引数の数を取得する
        int getParamNum() {
            return Params.size();
        }
};

/// 関数を表すAST
class FunctionAST {
    // 関数のプロトタイプ宣言
    PrototypeAST *Proto;
    // 関数のボディ
    FunctionStmtAST *Body;

    public:
        FunctionAST(PrototypeAST *proto, FunctionStmtAST *body) : Proto(proto), Body(body){}
        ~FunctionAST();

        // 関数名を取得する
        std::string getName() {
            return Proto -> getName();
        }

        // この関数のプロトタイプ宣言を取得する
        PrototypeAST *getPrototype() {
            return Proto;
        }

        // この関数のボディを取得
        FunctionStmtAST *getBody() {
            return Body;
        }
};

/// ソースコードを表すAST
class TranslationUnitAST {
    std::vector<PrototypeAST*> Prototypes;
    std::vector<FunctionAST*> Functions;

    public:
        TranslationUnitAST() {} ~TranslationUnitAST();

        // モジュールにプロトタイプ宣言を追加する
        bool addPrototype(PrototypeAST *proto);

        // モジュールに関数を追加する
        bool addFunction(FunctionAST *func);

        // モジュールが空か判定する
        bool empty();

        // i番目のプロトタイプ宣言を取得する
        PrototypeAST *getPrototype(int i) {
            if (i < Prototypes.size()) {
                return Prototypes.at(i);
            } else {
                return NULL;
            }
        }

        // i番目の関数を取得する
        FunctionAST *getFunction(int i) {
            if (i < Functions.size()) {
                return Functions.at(i);
            } else {
                return NULL;
            }
        }
};

// E: 関数とモジュール p76
