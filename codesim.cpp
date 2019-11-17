#include <clang-c/Index.h>

#include <iostream>
#include <string>
#include <getopt.h>
#include "Capted.h"

using namespace capted;

bool verbose = false;

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

	Node<StringNodeData> *parent = *(reinterpret_cast<Node<StringNodeData> **>(clientData));

	CXCursorKind cursorKind = clang_getCursorKind(cursor);

	Node<StringNodeData> *current = new Node<StringNodeData>(new StringNodeData(getCursorKindName(cursorKind)));

	parent->addChild(current);

	clang_visitChildren(cursor,
						treeBuilder,
						&current);

	return CXChildVisit_Continue;
}

Node<StringNodeData> *buildTree(char *filename)
{
	int displayDiagnostics = 0;
	if (verbose) {
		std::cerr << "Parsing " << filename << "...\n";
		displayDiagnostics = 1;
	}

	CXIndex index = clang_createIndex(0, displayDiagnostics);
	CXTranslationUnit translationUnit;

	translationUnit = clang_parseTranslationUnit(index, filename, 0, 0,
												 0, 0,
												 CXTranslationUnit_None);

	if (!translationUnit)
	{
		std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
		exit(-1);
	}

	CXCursor rootCursor = clang_getTranslationUnitCursor(translationUnit);

	CXCursorKind rootKind = clang_getCursorKind(rootCursor);
	unsigned int level = 0;
	if (verbose)
	{
		clang_visitChildren(rootCursor, visitor, &level);
	}

	Node<StringNodeData> *root = new Node<StringNodeData>(new StringNodeData(getCursorKindName(rootKind)));
	clang_visitChildren(rootCursor, treeBuilder, &root);

	clang_disposeTranslationUnit(translationUnit);
	clang_disposeIndex(index);

	return root;
}

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
		{"verbose", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0},
	};

	while ((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1)
	{
		switch (c)
		{
		case 'v':
			verbose = true;
			break;
		case 'h':
		case '?':
			printf("usage: codesim [-v|--verbose] [-h|--help] code1 code2\n");
			exit(EXIT_SUCCESS);
		default:
			; // not happen
		}
	}

	if (argc < optind + 2)
	{
		printf("usage: codesim [-v|--verbose] [-h|--help] code1 code2\n");
		exit(-1);
	}

	Node<StringNodeData> *n1 = buildTree(argv[optind++]);
	Node<StringNodeData> *n2 = buildTree(argv[optind]);

	float DistAB = computeEditDistance(n1, n2);

	Node<StringNodeData> *emptyTree = new Node<StringNodeData>(new StringNodeData(""));
	float DistA0 = computeEditDistance(n1, emptyTree);
	float DistB0 = computeEditDistance(n2, emptyTree);

	float similarity = 1 - DistAB / (DistA0 + DistB0);
	std::cout << similarity << std::endl;

	return 0;
}
