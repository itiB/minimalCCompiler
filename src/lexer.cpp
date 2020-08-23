#include "lexer.hpp"

/// トークン切り出し関数
/// @param 字句解析対象ファイル名
/// @return 切り出したトークンを格納したTokenStream
TokenStream *LexicalAnalysis(std::string input_filename) {
    TokenStream *tokens = new TokenStream();
    std::ifstream ifs;
    std::string cur_line;
    std::string token_str;
    int line_num = 0;
    bool iscomment = false;

    ifs.open(input_filename.c_str(), std::ios::in);
    if (!ifs) {
        return NULL;
    }

    while (ifs && getline(ifs, cur_line)) {
        char next_char;
        std::string line;
        Token *next_token;
        int index = 0;
        int length = cur_line.length();

        while (index < length) {
            next_char = cur_line.at(index++);

            // ｺﾒﾝﾄを読み飛ばす
            if (iscomment) {
                if ((length - index) < 2
                    || (cur_line.at(index) != '*')
                    || (cur_line.at(index++) += '/')) {
                        continue;
                } else {
                    iscomment = false;
                }
            }

            if (next_char == EOF) {
                // EOF
                token_str = EOF;
                next_token = new Token(token_str, TOK_EOF, line_num);

            } else if (isspace(next_char)) {
                // 空白
                continue;

            } else if (isalpha(next_char)) {
                // identifier
                token_str += next_char;
                next_char = cur_line.at(index++);
                while (isalnum(next_char)) {
                    // token_strに文字列を作る
                    token_str += next_char;
                    next_char = cur_line.at(index++);
                    if (index == length)
                        break;
                }
                index--;

                if (token_str == "int") {
                    next_token = new Token(token_str, TOK_INT, line_num);
                } else if (token_str == "return") {
                    next_token = new Token(token_str, TOK_RETURN, line_num);
                } else {
                    next_token = new Token(token_str, TOK_IDENTIFIER, line_num);
                }

            } else if (isdigit(next_char)) {
                // 数字
                if (next_char == '0') {
                    token_str += next_char;
                    next_token = new Token(token_str, TOK_DIGIT, line_num);
                } else {
                    token_str += next_char;
                    next_char = cur_line.at(index++);
                    while (isdigit(next_char)) {
                        token_str += next_char;
                        next_char = cur_line.at(index++);
                    }
                    next_token = new Token(token_str, TOK_DIGIT, line_num);
                    index--;
                }
            } else if (next_char == '/') {
                // ｺﾒﾝﾄまたは徐算演算子
                token_str += next_char;
                next_char = cur_line.at(index++);

                // ｺﾒﾝﾄの場合
                if (next_char == '/') {
                    break;

                } else if (next_char == '*') {
                    // ｺﾒﾝﾄの場合
                    iscomment = true;
                    continue;

                } else {
                    // 除算演算子
                    index--;
                    next_token = new Token(token_str, TOK_SYMBOL, line_num);
                }

            } else {
                // それ以外
                if (next_char == '*' ||
                    next_char == '+' ||
                    next_char == '-' ||
                    next_char == '=' ||
                    next_char == ';' ||
                    next_char == ',' ||
                    next_char == '(' ||
                    next_char == ')' ||
                    next_char == '{' ||
                    next_char == '}' ){
                        token_str += next_char;
                        next_token = new Token(token_str, TOK_SYMBOL, line_num);
                } else {
                    // 解析不能字句
                    fprintf(stderr, "unclear token: %c", next_char);
                    SAFE_DELETE(tokens);
                    return NULL;
                }
            }

            // Tokensに追加
            tokens -> pushToken(next_token);
            token_str.clear();
        }

        token_str.clear();
        line_num++;
    }

    // EOFの確認
    if (ifs.eof()) {
        tokens -> pushToken (
            new Token(token_str, TOK_EOF, line_num)
        );
    }

    // ｸﾛｰｽﾞ
    ifs.close();
    return tokens;
}


