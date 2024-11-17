#pragma once

#include <QtWidgets>

class Journal {
 public:
  QString eissn;
  QString title;
  QString jcr_abbr;
  QString iso_abbr;
  QString med_abbr;

  double ratio(const QString& query) const;
};

class SearchResult {
 public:
  double score;
  Journal journal;
  bool operator<(const SearchResult& other) const;
  bool operator>(const SearchResult& other) const;
  bool operator==(const SearchResult& other) const;
};

class TermItem {
 public:
  QList<SearchResult> search_results;
  QString name;
  QString abbr;
  int selected_index;
  TermItem(const QString& name, const QString& abbr);
  TermItem(const QString& name);
  double get_match_score();
  void search(bool update = true, double score_cutoff = 80);
  bool is_user_manually_inputted();
};

void load_all_journals();
QList<TermItem> load_term_list(const QString& file_name);
QList<SearchResult> search_journal(const QString& query);