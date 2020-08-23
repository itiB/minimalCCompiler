#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"

// http://ktanimoto.net/public/wordpress/2019/06/kitune-sandemowakaru-llvm-5-10made-ugokasu/
#include "llvm/Support/FileSystem.h"  //added
#include "llvm/Support/raw_ostream.h"  //added

#include "ast.hpp"
#include "codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"

/// 引数のオプション切り出しクラス
class OptionParser {
    private:
        std::string InputFilename;
        std::string OutputFilename;
        int Argc;
        char **Argv;

    public:
        OptionParser(int argc, char **argv):Argc(argc), Argv(argv) {}
        void printHelp() {
            // ヘルプ表示
            fprintf(stdout, "Compiler for DummyC...\n");
        }
        std::string getInputFileName() { return InputFilename; } // 入力ファイル名の取得
        std::string getOutputFileName() { return OutputFilename; } // 出力ファイル名の取得
        bool parseOption(); // オプション切り出しメソッド
};

/// オプション切り出しメソッド
/// @return 成功時: True, 失敗時: false
bool OptionParser::parseOption() {
    if (Argc < 2) {
        fprintf(stderr, "引数がたりません\n");
        return false;
    }

    for (int i = 1; i < Argc; i++) {
        if (Argv[i][0] == '-' && Argv[i][1] == 'o' && Argv[i][2] == '\0') {
            // output file name
            OutputFilename.assign(Argv[++i]);
        } else if (Argv[i][0] == '-' && Argv[i][1] == 'h' && Argv[i][2] == '\0') {
            printHelp();
            return false;
        } else if (Argv[i][0] == '-') {
            fprintf(stderr, "%s は不明なオプションです\n", Argv[i]);
            return false;
        } else {
            // inputFilename
            InputFilename.assign(Argv[i]);
        }
    }

    // Output filename
    std::string ifn = InputFilename;
    int len = ifn.length();
    if (OutputFilename.empty() && (len > 2) &&
        ifn[len - 3] == '.' && ((ifn[len - 2] == 'd' && ifn[len - 1] == 'c'))) {
        OutputFilename = std::string(ifn.begin(), ifn.end() - 3);
        OutputFilename += ".ll";
    } else if (OutputFilename.empty()) {
        OutputFilename = ifn;
        OutputFilename += ".ll";
    }
    return true;
}

/// main関数
/// OptionParserの呼び出し
/// 各種クラスの生成とメソッド呼び出し、コンパイルとファイル呼び出し
int main(int argc, char **argv) {
    llvm::InitializeNativeTarget(); // ホスト環境に合わせてネイティブターゲットを初期化
    // llvm::sys::PrintStackTraceOnErrorSignal(); // スタックトレースの出力
    llvm::sys::PrintStackTraceOnErrorSignal(*argv);
    llvm::PrettyStackTraceProgram X(argc, argv); // クラッシュした際に指定された引数をストリームに出力
    llvm::EnableDebugBuffering = true;

    OptionParser opt(argc, argv);
    if (!opt.parseOption())
        exit(1);

    // check
    if (opt.getInputFileName().length() == 0) {
        fprintf(stderr, "入力ファイルが指定されていません\n");
        exit(1);
    }

    // lex and parse
    // パーサクラスのインスタンスを生成
    Parser *parser = new Parser(opt.getInputFileName());
    
    // 構文解析、意味解析を行う
    if (!parser->doParse()) {
        fprintf(stderr, "err at parser or lexer\n");
        SAFE_DELETE(parser);
        exit(1);
    }

    // get AST
    TranslationUnitAST &tunit = parser->getAST();
    if (tunit.empty()) {
        fprintf(stderr, "TranslationUnit is empty");
        SAFE_DELETE(parser);
        exit(1);
    }

    // コード生成
    CodeGen *codegen = new CodeGen();
    if (!codegen->doCodeGen(tunit, opt.getInputFileName())) {
        fprintf(stderr, "err at codegen\n");
        SAFE_DELETE(parser);
        exit(1);
    }

    // get Module
    llvm::Module &mod = codegen->getModule();
    if (mod.empty()) {
        fprintf(stderr, "Module is empty\n");
        SAFE_DELETE(parser);
        exit(1);
    }

    // ファイル出力
    llvm::legacy::PassManager pm;
    // std::string error;
    std::error_code ec;

    // raw_fd_ostream
    // raw_fd_ostream(const char *Filename, std::string &ErrorInfo, unsigned Flags=0)
    //  - filename: 出力先ファイル名
    //  - errorinfo: エラー情報を格納するstring
    //  - flags: ファイルを開くオプション
    // llvm::raw_fd_ostream raw_stream(opt.getOutputFileName().c_str(), error);
    llvm::raw_fd_ostream raw_stream(opt.getOutputFileName().c_str(), ec);

    // PrimtModulePassはPassManagerのaddメソッドで登録、runで適用
    // llvm::CreatePrintModulePass
    // ModulePass* llvm::createPrintModulePass(raw_ostrean * OS, bool DeleteStream = false, const std::string &Banner="")
    // - OS: 出力ストリームの指定
    // - DeleteStream: パスを実行後に第一引数で渡したストリームをDeleteするかしないか
    // - Banner: ストリームの先頭にBannerに指定した文字列が出力される
    // pm.add(llvm::createPrintModulePass(&raw_stream));
    pm.add(llvm::createPrintModulePass(raw_stream));
    pm.run(mod);
    raw_stream.close();

    // 終了処理
    SAFE_DELETE(parser);
    SAFE_DELETE(codegen);

    return 0;
}
