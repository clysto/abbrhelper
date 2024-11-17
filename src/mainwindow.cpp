#include "mainwindow.hpp"

#include <QFuture>
#include <QThread>
#include <QtConcurrent>

#include "utils.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow{parent} {
  center_widget = new QWidget(this);
  setWindowTitle("Journal Abbr Helper");
  setCentralWidget(center_widget);
  setMinimumSize(800, 600);

  auto vbox = new QVBoxLayout(center_widget);
  auto hbox = new QHBoxLayout();
  info_panel = new QWidget();
  info_panel->setFixedWidth(270);
  info_panel_layout = new QVBoxLayout(info_panel);
  abbr_input = new QLineEdit();
  abbr_input->hide();

  terms_table = new QTableView(center_widget);
  terms_model = new TermsTableModel(terms_table);
  terms_table->setModel(terms_model);
  terms_table->verticalHeader()->hide();
  terms_table->setColumnWidth(0, 200);
  terms_table->setColumnWidth(1, 200);
  terms_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  terms_table->setSelectionMode(QAbstractItemView::SingleSelection);

  auto fetch_button = new QPushButton("Fetch Abbreviations");
  vbox->addWidget(fetch_button);
  vbox->addLayout(hbox);
  hbox->addWidget(terms_table);
  hbox->addWidget(info_panel);
  info_panel_layout->addWidget(new QLabel("Matched Journals"));
  info_panel_layout->setAlignment(Qt::AlignLeft);
  matched_select = new QComboBox();
  matched_select->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  info_panel_layout->addWidget(matched_select);

  journal_info = new QWidget();
  journal_info->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); 
  journal_info_layout = new QFormLayout(journal_info);

  QList<QString> label_names = {"Journal Name:", "Online ISSN:", "ISO Abbr:", "JCR Abbr:", "MED Abbr:"};

  for (int i = 0; i < label_names.size(); ++i) {
    auto label = new QLineEdit();
    label->setReadOnly(true);
    journal_info_layout->addRow(label_names[i], label);
  }
  journal_info_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  info_panel_layout->addWidget(journal_info);
  info_panel_layout->addWidget(abbr_input);
  info_panel_layout->addStretch();

  // build menu
  auto menu = new QMenuBar(this);
  auto fileMenu = new QMenu("File", this);
  auto open_action = new QAction("Open", this);
  auto save_action = new QAction("Save", this);
  open_action->setShortcut(QKeySequence::Open);
  save_action->setShortcut(QKeySequence::SaveAs);
  fileMenu->addAction(open_action);
  fileMenu->addAction(save_action);
  menu->addMenu(fileMenu);
  setMenuBar(menu);

  // connect signals
  connect(open_action, &QAction::triggered, this, &MainWindow::open_file);
  connect(save_action, &QAction::triggered, this, &MainWindow::save_file);
  connect(fetch_button, &QPushButton::clicked, terms_model, &TermsTableModel::fetch_abbrs);
  auto selection_model = terms_table->selectionModel();
  connect(selection_model, &QItemSelectionModel::selectionChanged, this, &MainWindow::select_term);
  connect(abbr_input, &QLineEdit::textChanged, this, &MainWindow::change_abbr);
}

void MainWindow::open_file() {
  auto file_name = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(), "TXT Files (*.txt)");
  if (file_name.isEmpty()) {
    return;
  }
  terms_model->setTermItems(std::move(load_term_list(file_name)));
};

void MainWindow::save_file() {
  auto file_name = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath(), "TXT Files (*.txt)");
  if (file_name.isEmpty()) {
    return;
  }
  terms_model->save_to_file(file_name);
};

void MainWindow::select_term(const QItemSelection& selected, const QItemSelection& deselected) {
  auto indexes = selected.indexes();
  if (indexes.size() == 0) {
    return;
  }

  disconnect(matched_select, &QComboBox::currentIndexChanged, this, &MainWindow::change_matched);
  auto index = indexes[0].row();
  auto term = terms_model->get_term_item(index);
  abbr_input->setText(term.abbr);
  matched_select->clear();
  for (auto& result : term.search_results) {
    matched_select->addItem(result.journal.title);
  }
  matched_select->addItem("---None of the above---");
  if (term.is_user_manually_inputted()) {
    matched_select->setCurrentIndex(matched_select->count() - 1);
  } else {
    matched_select->setCurrentIndex(term.selected_index);
    auto journal = term.search_results[term.selected_index].journal;
    update_journal_info(journal);
  }
  connect(matched_select, &QComboBox::currentIndexChanged, this, &MainWindow::change_matched);
  change_matched(matched_select->currentIndex());
};

void MainWindow::update_journal_info(const Journal& journal) {
  QList<QString> labels = {journal.title, journal.eissn, journal.iso_abbr, journal.jcr_abbr, journal.med_abbr};
  for (int i = 0; i < labels.size(); i++) {
    auto label = static_cast<QLineEdit*>(journal_info_layout->itemAt(i, QFormLayout::FieldRole)->widget());
    label->setText(labels[i]);
    label->setCursorPosition(0);
  }
};

void MainWindow::change_matched(int index) {
  if (matched_select->count() == 0) {
    return;
  }
  auto term_index = terms_table->selectionModel()->selectedIndexes()[0].row();
  auto& term = terms_model->get_term_item(term_index);
  if (index == matched_select->count() - 1) {
    term.selected_index = -1;
    update_journal_info(Journal{"", "", "", "", ""});
    journal_info->hide();
    abbr_input->show();
  } else {
    term.selected_index = index;
    term.abbr = term.search_results[index].journal.iso_abbr;
    auto journal = term.search_results[index].journal;
    update_journal_info(journal);
    journal_info->show();
    abbr_input->hide();
  };
  auto term2 = terms_model->get_term_item(term_index);
};

void MainWindow::change_abbr(const QString& abbr) {
  auto term_index = terms_table->selectionModel()->selectedIndexes()[0].row();
  auto& term = terms_model->get_term_item(term_index);
  term.abbr = abbr;
};

void TermsTableModel::fetch_abbrs() {
  QProgressDialog* progressDialog = new QProgressDialog("Searching the database...", "Cancel", 0, 0, nullptr);
  progressDialog->setWindowModality(Qt::ApplicationModal);
  progressDialog->setCancelButton(nullptr);
  progressDialog->setWindowTitle("Please Wait");
  progressDialog->show();
  QFuture<void> future = QtConcurrent::run([this, progressDialog]() {
    QList<TermItem> updated_items = term_items;
    for (auto& term_item : updated_items) {
      term_item.search();
    }
    QMetaObject::invokeMethod(this, [this, progressDialog, updated_items]() {
      beginResetModel();
      term_items = updated_items;
      endResetModel();
      progressDialog->close();
      progressDialog->deleteLater();
    });
  });
}

TermsTableModel::TermsTableModel(QObject* parent) : QAbstractTableModel{parent} {}

int TermsTableModel::rowCount(const QModelIndex& parent) const { return term_items.size(); }

int TermsTableModel::columnCount(const QModelIndex& parent) const { return 2; }

QVariant TermsTableModel::data(const QModelIndex& index, int role) const {
  auto term = term_items[index.row()];
  if (role == Qt::DisplayRole) {
    if (index.column() == 0) {
      return term.name;
    } else if (index.column() == 1) {
      return term.abbr;
    }
  } else if (role == Qt::BackgroundRole) {
    if (index.column() == 1) {
      if (term.is_user_manually_inputted()) {
        return QVariant();
      }
      int score = term.get_match_score();
      if (score < 50) {
        return QBrush(QColor(255, 0, 0));
      } else {
        int adjusted_score = score - 50;
        int red = 255 - (adjusted_score * 255 / 50);
        int green = adjusted_score * 255 / 50;
        return QBrush(QColor(red, green, 0));
      }
    }
  }
  return QVariant();
}

QVariant TermsTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == 0) {
      return "Journal Name";
    } else if (section == 1) {
      return "ISO Abbr";
    }
  }
  return QVariant{};
}

void TermsTableModel::setTermItems(const QList<TermItem>&& term_items) {
  beginResetModel();
  this->term_items = term_items;
  endResetModel();
}

void TermsTableModel::save_to_file(const QString& file_name) {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(nullptr, "Error", "Failed to open file: " + file_name);
    return;
  }

  QTextStream out(&file);
  for (auto& term : term_items) {
    out << term.name << "\t" << term.abbr << "\n";
  }
  file.close();

  QMessageBox::information(nullptr, "Success", "File saved successfully: " + file_name);
}

TermItem& TermsTableModel::get_term_item(int row) { return term_items[row]; }
