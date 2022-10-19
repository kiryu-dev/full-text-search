#include <libfts/indexer.hpp>

#include <picosha2/picosha2.h>

#include <filesystem>
#include <fstream>

namespace libfts {

void IndexBuilder::add_document(
    size_t document_id,
    const std::string &text,
    const ParserConfiguration &config) {
    /* if a document with this id hasn't yet been added, then add it, otherwise
     * the method will simply do nothing */
    if (index_.docs_.find(document_id) == index_.docs_.end()) {
        index_.docs_[document_id] = text;
        std::vector<ParsedString> parsed_text = parse(text, config);
        for (const auto &word : parsed_text) {
            for (const auto &term : word.ngrams_) {
                index_.entries_[term][document_id].push_back(
                    word.text_position_);
            }
        }
    }
}

static std::string convert_entries(const Term &term, const Entry &entry) {
    std::string result = term + " " + std::to_string(entry.size()) + " ";
    for (const auto &[docs_id, position] : entry) {
        result += std::to_string(docs_id) + " " +
            std::to_string(position.size()) + " ";
        for (const auto &pos : position) {
            result += std::to_string(pos) + " ";
        }
    }
    return result;
}

void TextIndexWriter::write(const std::string &path, const Index &index) {
    std::filesystem::create_directories(path + "docs/");
    std::filesystem::create_directories(path + "entries/");
    for (const auto &[docs_id, docs] : index.get_docs()) {
        std::fstream file(
            path + "docs/" + std::to_string(docs_id), std::fstream::out);
        file << docs;
    }
    for (const auto &[terms, entries] : index.get_entries()) {
        std::string hash_hex_term = generate_hash(terms);
        std::fstream file(path + "entries/" + hash_hex_term, std::fstream::out);
        file << convert_entries(terms, entries);
    }
}

std::string generate_hash(const std::string &term) {
    std::string hash_hex_term;
    picosha2::hash256_hex_string(term, hash_hex_term);
    hash_hex_term.resize(FIRST_NECESSARY_BYTES);
    return hash_hex_term;
}

void parse_entry(
    const std::string &path, [[maybe_unused]] std::map<Term, Entry> &entries) {
    std::fstream file(path, std::fstream::in);
    std::string term;
    file >> term;
    size_t doc_count = 0;
    file >> doc_count;
    Entry entry;
    for (size_t i = 0; i < doc_count; ++i) {
        size_t document_id = 0;
        file >> document_id;
        Pos position;
        size_t pos_count = 0;
        file >> pos_count;
        for (size_t j = 0; j < pos_count; ++j) {
            size_t pos_num = 0;
            file >> pos_num;
            position.push_back(pos_num);
        }
        entry[document_id] = position;
    }
    entries[term] = entry;
}

} // namespace libfts