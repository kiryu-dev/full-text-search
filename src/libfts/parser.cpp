#include <libfts/parser.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>

namespace libfts {

ParserConfiguration::ParserConfiguration(
    std::set<std::string> stop_words,
    const NgramLength &ngram_length,
    double cutoff_factor)
    : stop_words_{std::move(stop_words)}, ngram_length_{ngram_length},
      cutoff_factor_{cutoff_factor} {
    if (ngram_length.min_ >= ngram_length.max_) {
        throw ConfigurationException(
            "maximum ngram length must be greater than minimum ngram length");
    }
}

ParserConfiguration load_config(const std::filesystem::path &filename) {
    std::ifstream file(filename);
    const auto config = nlohmann::json::parse(file, nullptr, false);
    if (config.is_discarded()) {
        throw ConfigurationException("incorrect configuration format");
    }
    const auto min_ngram_length = config["minimum_ngram_length"].get<int>();
    const auto max_ngram_length = config["maximum_ngram_length"].get<int>();
    if (min_ngram_length < 0 || max_ngram_length < 0) {
        throw ConfigurationException("ngram lengths must be unsigned integers");
    }
    const auto cutoff_factor = config["cutoff_factor"].get<double>();
    if (cutoff_factor < 0 || cutoff_factor >= 1) {
        throw ConfigurationException(
            "cutoff factor must be in the range [0,1)");
    }
    const auto stop_words = config["stop_words"].get<std::set<std::string>>();
    return {
        stop_words,
        {static_cast<std::size_t>(min_ngram_length),
         static_cast<std::size_t>(max_ngram_length)},
        cutoff_factor};
}

static void remove_punct(std::string &str) {
    str.erase(
        std::remove_if(
            str.begin(),
            str.end(),
            [](unsigned char chr) { return std::ispunct(chr); }),
        str.end());
}

static void string_to_lower(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char chr) {
        return std::tolower(chr);
    });
}

static std::vector<std::string> split_string(std::string &str) {
    std::vector<std::string> words;
    auto first = std::find_if_not(str.begin(), str.end(), isspace);
    while (first != str.end()) {
        auto last = std::find_if(first, str.end(), isspace);
        std::string word = str.substr(first - str.begin(), last - first);
        words.push_back(word);
        first = std::find_if_not(last, str.end(), isspace);
    }
    return words;
}

static std::vector<std::string> remove_stop_words(
    const std::vector<std::string> &text,
    const std::set<std::string> &stop_words) {
    std::vector<std::string> text_without_stop_words;
    std::copy_if(
        text.begin(),
        text.end(),
        std::back_inserter(text_without_stop_words),
        [&stop_words](const std::string &word) {
            return stop_words.find(word) == stop_words.end();
        });
    return text_without_stop_words;
}

static std::vector<ParsedString> generate_ngrams(
    const std::vector<std::string> &words, const ParserConfiguration &config) {
    std::vector<ParsedString> parsed_query;
    for (std::size_t i = 0; i < words.size(); ++i) {
        ParsedString word;
        for (std::size_t j = config.get_min_ngram_length();
             j <= config.get_max_ngram_length();
             ++j) {
            if (words[i].length() < j) {
                break;
            }
            word.ngrams_.push_back(words[i].substr(0, j));
        }
        if (!word.ngrams_.empty()) {
            word.text_position_ = i;
            parsed_query.push_back(word);
        }
    }
    return parsed_query;
}

std::vector<ParsedString>
parse(std::string text, const ParserConfiguration &config) {
    remove_punct(text);
    string_to_lower(text);
    std::vector<std::string> words = split_string(text);
    words = remove_stop_words(words, config.get_stop_words());
    return generate_ngrams(words, config);
}

} // namespace libfts