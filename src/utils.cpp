#include "utils.hpp"

#include <queue>
#include <rapidfuzz/fuzz.hpp>

QList<Journal> db_journals;

QList<TermItem> load_term_list(const QString& file_name) {
  QFile file{file_name};
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  QTextStream in{&file};
  QList<TermItem> term_items;
  QSet<QString> seen_names;

  while (!in.atEnd()) {
    auto line = in.readLine();
    auto parts = line.split("\t");
    if (parts.size() >= 2) {
      if (!seen_names.contains(parts[0])) {
        term_items.append(TermItem(parts[0], parts[1]));
        seen_names.insert(parts[0]);
      }
    } else {
      if (!seen_names.contains(parts[0])) {
        term_items.append(TermItem(parts[0]));
        seen_names.insert(parts[0]);
      }
    }
  }
  return term_items;
}

void load_all_journals() {
  QFile file(":/resources/journals.csv");
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }
  QTextStream in{&file};
  in.readLine();
  while (!in.atEnd()) {
    auto line = in.readLine();
    auto parts = line.split(",");
    Journal journal;
    journal.eissn = parts[0];
    journal.title = parts[1];
    journal.jcr_abbr = parts[2];
    journal.iso_abbr = parts[3];
    journal.med_abbr = parts[4];
    db_journals.append(journal);
  }
}

QList<SearchResult> search_journal(const QString& query) {
  auto compare = [](const SearchResult& a, const SearchResult& b) { return a > b; };
  std::priority_queue<SearchResult, std::vector<SearchResult>, decltype(compare)> matches(compare);

  for (const auto& journal : db_journals) {
    auto score = journal.ratio(query);
    matches.push(SearchResult{score, journal});

    if (matches.size() > 5) {
      matches.pop();
    }
  }

  auto results = QList<SearchResult>();
  while (!matches.empty()) {
    results.prepend(matches.top());
    matches.pop();
  }

  return results;
}

bool SearchResult::operator<(const SearchResult& other) const { return score < other.score; }

bool SearchResult::operator>(const SearchResult& other) const { return score > other.score; }

bool SearchResult::operator==(const SearchResult& other) const { return score == other.score; }

double Journal::ratio(const QString& query) const {
  auto query_ = query.toLower();
  auto s1 = rapidfuzz::fuzz::ratio(title.toLower().toStdString(), query_.toStdString());
  auto s2 = rapidfuzz::fuzz::ratio(jcr_abbr.toLower().toStdString(), query_.toStdString());
  auto s3 = rapidfuzz::fuzz::ratio(iso_abbr.toLower().toStdString(), query_.toStdString());
  auto s4 = rapidfuzz::fuzz::ratio(med_abbr.toLower().toStdString(), query_.toStdString());
  return std::max({s1, s2, s3, s4});
}

TermItem::TermItem(const QString& name, const QString& abbr) : name(name), abbr(abbr), selected_index(-1) {}
TermItem::TermItem(const QString& name) : name(name), selected_index(-1) {}
void TermItem::search(bool update, double score_cutoff) {
  search_results = search_journal(name);
  if (update && abbr.isEmpty()) {
    for (int i = 0; i < search_results.size(); i++) {
      if (search_results[i].score >= score_cutoff) {
        selected_index = i;
        break;
      }
    }
    if (selected_index != -1) {
      abbr = search_results[selected_index].journal.iso_abbr;
    }
  }
}

double TermItem::get_match_score() {
  for (const auto& result : search_results) {
    if (result.journal.iso_abbr == abbr) {
      return result.score;
    }
  }
  return -1;
}
bool TermItem::is_user_manually_inputted() { return selected_index == -1; }