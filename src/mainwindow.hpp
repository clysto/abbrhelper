#pragma once

#include <QtWidgets>

#include "utils.hpp"

class TermsTableModel : public QAbstractTableModel {
 private:
  QList<TermItem> term_items;

 public:
  TermsTableModel(QObject* parent = nullptr);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  void setTermItems(const QList<TermItem>&& term_items);
  TermItem& get_term_item(int row);
  void save_to_file(const QString& file_name);
 public slots:
  void fetch_abbrs();
};

class MainWindow : public QMainWindow {
 private:
  QWidget* center_widget;
  QTableView* terms_table;
  TermsTableModel* terms_model;
  QWidget* info_panel;
  QVBoxLayout* info_panel_layout;
  QWidget* journal_info;
  QFormLayout* journal_info_layout;
  QComboBox* matched_select;
  QLineEdit* abbr_input;
  void update_journal_info(const Journal& journal);

 public:
  MainWindow(QWidget* parent = nullptr);

 public slots:
  void open_file();
  void save_file();
  void select_term(const QItemSelection& selected, const QItemSelection& deselected);
  void change_matched(int index);
  void change_abbr(const QString& abbr);
};
