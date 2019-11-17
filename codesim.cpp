#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

#include <clang-c/Index.h>
#include "Capted.h"

using namespace capted;

bool verbose = false;

// CXStrings need to be disprosed
// For convinience, we convert them to std::string.
std::string getCursorKindName(CXCursorKind cursorKind)
{
	CXString kindName = clang_getCursorKindSpelling(cursorKind);
	std::string result = clang_getCString(kindName);

	clang_disposeString(kindName);
	return result;
}

std::string getCursorSpelling(CXCursor cursor)
{
	CXString cursorSpelling = clang_getCursorSpelling(cursor);
	std::string result = clang_getCString(cursorSpelling);

	clang_disposeString(cursorSpelling);
	return result;
}


CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
	// skip those not in main file. for example those #include<...>
	CXSourceLocation location = clang_getCursorLocation(cursor);
	if (clang_Location_isFromMainFile(location) == 0)
		return CXChildVisit_Continue;

	CXCursorKind cursorKind = clang_getCursorKind(cursor);

	unsigned int curLevel = *(reinterpret_cast<unsigned int *>(clientData));
	unsigned int nextLevel = curLevel + 1;

	std::cout << std::string(curLevel, '-') << " " << getCursorKindName(cursorKind) << " (" << getCursorSpelling(cursor) << ")\n";

	clang_visitChildren(cursor,
						visitor,
						&nextLevel);

	return CXChildVisit_Continue;
}

CXChildVisitResult treeBuilder(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
	CXSourceLocation location = clang_getCursorLocation(cursor);
	if (clang_Location_isFromMainFile(location) == 0)
		return CXChildVisit_Continue;

	// create a Node for current AST structure
	CXCursorKind cursorKind = clang_getCursorKind(cursor);
	auto *current = new Node<StringNodeData>(new StringNodeData(getCursorKindName(cursorKind)));

	// link it to the main tree
	Node<StringNodeData> *parent = *(reinterpret_cast<Node<StringNodeData> **>(clientData));
	parent->addChild(current);

	// continue to visit its children nodes.
	clang_visitChildren(cursor,
						treeBuilder,
						&current);

	return CXChildVisit_Continue;
}

Node<StringNodeData> *buildTree(std::string& filename)
{
	int displayDiagnostics = 0;
	if (verbose) {
		std::cerr << "Parsing " << filename << "...\n";
		displayDiagnostics = 1;
	}

	CXIndex index = clang_createIndex(0, displayDiagnostics);
	CXTranslationUnit translationUnit;

	// pass args to clang to control its behavior
	// to help clang find header files, e.g. stddef.h, put clang's builtin headers in.
	// in latest versions, doesn't need this anymore. Only care for clang-6.0
	constexpr const char* defaultArguments[] = {
	  "-std=c++11",
	  "-I/usr/lib/llvm-6.0/lib/clang/6.0.0/include"
	};
	translationUnit = clang_parseTranslationUnit(index,
												 filename.c_str(),
												 defaultArguments,
												 std::extent<decltype(defaultArguments)>::value,
												 nullptr, 0,
												 CXTranslationUnit_None);

	if (!translationUnit)
	{
		std::cerr << "Unable to parse translation unit " << filename << std::endl;
		exit(EXIT_FAILURE);
	}

	CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);

	CXCursorKind rootKind = clang_getCursorKind(rootCursor);
	unsigned int level = 0;
	if (verbose)
	{
		clang_visitChildren(rootCursor, visitor, &level);
	}

	auto *root = new Node<StringNodeData>(new StringNodeData(getCursorKindName(rootKind)));
	clang_visitChildren(rootCursor, treeBuilder, &root);

	clang_disposeTranslationUnit(translationUnit);
	clang_disposeIndex(index);

	return root;
}

// because the restricts in Capted library, we need this helper function
float computeEditDistance(Node<StringNodeData> *t1, Node<StringNodeData> *t2) {
	StringCostModel costModel;
	Apted<StringNodeData> algorithm(&costModel);

	float compDist = algorithm.computeEditDistance(t1, t2);

	return compDist;
}

int main(int argc, char **argv)
{
	const char *optstring = "vh";
	int c;
	struct option opts[] = {
		{"verbose", 0, nullptr, 'v'},
		{"help", 0, nullptr, 'h'},
		{nullptr, 0, nullptr, 0},
	};

	while ((c = getopt_long(argc, argv, optstring, opts, nullptr)) != -1)
	{
		switch (c)
		{
		case 'v':
			verbose = true;
			break;
		case 'h':
		case '?':
			std::cout << "usage: codesim [-v|--verbose] [-h|--help] code1 code2" << std::endl;
			exit(EXIT_SUCCESS);
		default:
			; // will not happen
		}
	}

	// make sure there are enough input source files
	std::string f1, f2;
	if (argc < optind + 2)
	{
		std::cout << "usage: codesim [-v|--verbose] [-h|--help] code1 code2" << std::endl;
		exit(EXIT_FAILURE);
	} else
	{
		f1 = argv[optind++];
		f2 = argv[optind];
		// check if the files can be opened
		std::ifstream in1(f1);
		std::ifstream in2(f2);

		if (!in1.good()) {
			std::cout << "Can't open the file: " << f1 << std::endl;
			exit(EXIT_FAILURE);
		}
		if (!in2.good()) {
			std::cout << "Can't open the file: " << f2 << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	Node<StringNodeData> *n1 = buildTree(f1);
	Node<StringNodeData> *n2 = buildTree(f2);

	float DistAB = computeEditDistance(n1, n2);

	auto *emptyTree = new Node<StringNodeData>(new StringNodeData(""));
	float DistA0 = computeEditDistance(n1, emptyTree);
	float DistB0 = computeEditDistance(n2, emptyTree);

	float similarity = 1 - DistAB / (DistA0 + DistB0);
	std::cout << similarity << std::endl;

	return 0;
}
