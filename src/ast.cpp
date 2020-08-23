#include "ast.hpp"

/// デストラクタ
TokenStream::~TokenStream() {
    for (int i = 0; i < Tokens.size(); i++) {
        SAFE_DELETE(Tokens[i]);
    }
    Tokens.clear();
}

/// トークンの取得
/// @return CureIndex番目のToken
Token TokenStream::getToken() {
    return *Tokens[CurIndex];
}

/// インデックスを1つ増やして次のトークンに進める
/// @return 成功時: true, 失敗時: false
bool TokenStream::getNextToken() {
    int size = Tokens.size();
    if (--size <= CurIndex) {
        return false;
    } else {
        CurIndex++;
        return true;
    }
}

/// インデックスをtimes回戻す
bool TokenStream::ungetToken(int times) {
    for (int i = 0; i < times; i++) {
        if (CurIndex == 0) {
            return false;
        } else {
            CurIndex--;
        }
    }
    return true;
}

/// 格納されたトークン一覧の表示
bool TokenStream::printTokens() {
    std::vector<Token*>::iterator titer = Tokens.begin();
    while (titer != Tokens.end()) {
        fprintf(stdout, "%d: ", (*titer) -> getTokenType());
        if ((*titer) -> getTokenType() != TOK_EOF) {
            fprintf(stdout, "%s\n", (*titer) -> getTokensString().c_str());
        }
        ++titer;
    }
    return true;
}

// E: TokenStreamクラスの定義 p64
