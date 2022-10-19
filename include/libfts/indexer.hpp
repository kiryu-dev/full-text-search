#pragma once

#include <libfts/parser.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace libfts {

const size_t c_term_hash_size = 6;

using DocId = size_t;
using Pos = std::vector<size_t>;
using Entry = std::map<DocId, Pos>;
using Term = std::string;
using Doc = std::string;

class IndexBuilder;

class Index {
    friend IndexBuilder;
    std::map<DocId, Doc> docs_;
    std::map<Term, Entry> entries_;

public:
    Index() = default;
    Index(std::map<DocId, Doc> docs, std::map<Term, Entry> entries)
        : docs_(std::move(docs)), entries_(std::move(entries)) {}
    std::map<DocId, Doc> get_docs() const { return docs_; }
    std::map<Term, Entry> get_entries() const { return entries_; }
};

class IndexBuilder {
    Index index_;

public:
    void add_document(
        size_t document_id,
        const std::string &text,
        const ParserConfiguration &config);
    Index get_index() { return {index_.docs_, index_.entries_}; }
};

struct TextIndexWriter {
    static void write(const std::filesystem::path &path, const Index &index);
};

std::string generate_hash(const std::string &Term);
void parse_entry(const std::string &path, std::map<Term, Entry> &entries);

} // namespace libfts