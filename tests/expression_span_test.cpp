#include <kasm/kasmlexer.hpp>
#include <kasm/kasmparser.hpp>

#include <iostream>

bool hasSpan(const Node& node, std::size_t begin, std::size_t end) {
    return node.span.begin == begin && node.span.end == end;
}

int main() {
    KasmLexer lexer;
    const auto tokens = lexer.tokenize("  12 + (3 * 4)");
    KasmParser parser(tokens);
    Expression expression;
    std::string error;
    if (!parser.parse_expression_tree(expression, error)) {
        std::cerr << "Could not parse expression: " << error << '\n';
        return 1;
    }
    const Node& sum = expression.nodes[expression.root];
    const Node& left = expression.nodes[sum.left];
    const Node& product = expression.nodes[sum.right];
    if (sum.kind != "+" || !hasSpan(sum, 2, 14) ||
        left.kind != "number" || !hasSpan(left, 2, 4) ||
        product.kind != "*" || !hasSpan(product, 7, 14) ||
        !hasSpan(expression.nodes[product.left], 8, 9) ||
        !hasSpan(expression.nodes[product.right], 12, 13)) {
        for (const Node& node : expression.nodes)
            std::cerr << node.kind << " [" << node.span.begin << ", " << node.span.end << ") children " << node.left << ',' << node.right << '\n';
        for (const Token& token : tokens)
            std::cerr << "token " << token.text << " [" << token.span.begin << ", " << token.span.end << ")\n";
        std::cerr << "Number, operator, or grouped expression span was incorrect\n";
        return 1;
    }

    KasmLexer positionLexer;
    const auto positionTokens = positionLexer.tokenize("$ + $$");
    KasmParser positionParser(positionTokens);
    Expression positionExpression;
    if (!positionParser.parse_expression_tree(positionExpression, error)) {
        std::cerr << "Could not parse position expression: " << error << '\n';
        return 1;
    }
    const Node& positionSum = positionExpression.nodes[positionExpression.root];
    if (!hasSpan(positionSum, 0, 6) ||
        !hasSpan(positionExpression.nodes[positionSum.left], 0, 1) ||
        !hasSpan(positionExpression.nodes[positionSum.right], 4, 6)) {
        std::cerr << "Position expression span was incorrect\n";
        return 1;
    }
    KasmLexer addressLexer;
    const auto addressTokens = addressLexer.tokenize("ebx sum ecx mul 4");
    KasmParser addressParser(addressTokens);
    Expression address;
    if (!addressParser.parse_expression_tree(address, error)) {
        std::cerr << "Could not parse address expression: " << error << '\n';
        return 1;
    }
    const Node& addressSum = address.nodes[address.root];
    const Node& base = address.nodes[addressSum.left];
    const Node& scaledIndex = address.nodes[addressSum.right];
    const Node& index = address.nodes[scaledIndex.left];
    const Node& scale = address.nodes[scaledIndex.right];
    if (addressSum.kind != "+" || base.kind != "identifier" || base.text != "ebx" ||
        scaledIndex.kind != "*" || index.kind != "identifier" || index.text != "ecx" ||
        scale.kind != "number" || scale.value != 4) {
        std::cerr << "Address expression tree was incorrect\n";
        return 1;
    }
    std::uint64_t value = 0;
    error.clear();
    if (addressParser.evaluate(address, address.root, value, error) ||
        error != "Cannot evaluate identifier: ebx") {
        std::cerr << "Identifier evaluation did not fail safely: " << error << '\n';
        return 1;
    }
    return 0;
}
