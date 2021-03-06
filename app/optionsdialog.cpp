/*
 * Copyright 2013 Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "optionsdialog.h"
#include "ui_optionsdialog.h"

#include <QFontComboBox>
#include <QItemEditorFactory>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QAction>
//#include <QDebug>
#include <QFileDialog>

#include <snippets/snippetcollection.h>
#include "options.h"
#include "snippetstablemodel.h"

class KeySequenceTableItem : public QTableWidgetItem
{
public:
    KeySequenceTableItem (const QKeySequence &keySequence) : 
        QTableWidgetItem(QTableWidgetItem::UserType + 1),
        m_keySequence(keySequence)
    {
    }

    QVariant data(int role) const
    {
        switch (role) {
            case Qt::DisplayRole:
                return m_keySequence.toString();
            case Qt::EditRole:
                return m_keySequence;
            default:
                return QVariant();
        }
    }

    void setData(int role, const QVariant &data)
    {
        if (role == Qt::EditRole)
            m_keySequence = data.value<QKeySequence>();

        QTableWidgetItem::setData(role, data);
    }

private:
    QKeySequence m_keySequence;
};

class KeySequenceEditFactory : public QItemEditorCreatorBase
{
public:
    QWidget *createWidget(QWidget *parent) const
    {
        return new QKeySequenceEdit(parent);
    }

    QByteArray valuePropertyName() const
    {
        return QByteArrayLiteral("keySequence");
    }
};


OptionsDialog::OptionsDialog(Options *opt, SnippetCollection *collection, const QVector<QAction*> &acts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    options(opt),
    snippetCollection(collection),
    actions(acts)
{
    ui->setupUi(this);

    ui->tabWidget->setIconSize(QSize(24, 24));
    ui->tabWidget->setTabIcon(0, QIcon(QStringLiteral("fa-cog.fontawesome")));
    ui->tabWidget->setTabIcon(1, QIcon(QStringLiteral("fa-file-text-o.fontawesome")));
    ui->tabWidget->setTabIcon(2, QIcon(QStringLiteral("fa-html5.fontawesome")));
    ui->tabWidget->setTabIcon(3, QIcon(QStringLiteral("fa-globe.fontawesome")));
    ui->tabWidget->setTabIcon(4, QIcon(QStringLiteral("fa-puzzle-piece.fontawesome")));
    ui->tabWidget->setTabIcon(5, QIcon(QStringLiteral("fa-keyboard-o.fontawesome")));

    const auto sizes = QFontDatabase::standardSizes();
    for (int size : sizes) {
        ui->sizeComboBox->addItem(QString().setNum(size));
        ui->defaultSizeComboBox->addItem(QString().setNum(size));
        ui->defaultFixedSizeComboBox->addItem(QString().setNum(size));
    }

    ui->portLineEdit->setValidator(new QIntValidator(0, 65535));
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);

    ui->snippetTableView->setModel(new SnippetsTableModel(snippetCollection, ui->snippetTableView));
    connect(ui->snippetTableView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &OptionsDialog::currentSnippetChanged);
    connect(ui->snippetTextEdit, &QPlainTextEdit::textChanged, this, &OptionsDialog::snippetTextChanged);

    setupShortcutsTable();

    // read configuration state
    readState();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::done(int result)
{
    if (result == QDialog::Accepted) {
        // save configuration state
        saveState();
    }

    QDialog::done(result);
}

void OptionsDialog::manualProxyRadioButtonToggled(bool checked)
{
    ui->hostLineEdit->setEnabled(checked);
    ui->portLineEdit->setEnabled(checked);
    ui->userNameLineEdit->setEnabled(checked);
    ui->passwordLineEdit->setEnabled(checked);
}

void OptionsDialog::currentSnippetChanged(const QModelIndex &current, const QModelIndex &)
{
    const Snippet snippet = snippetCollection->at(current.row());

    // update text edit for snippet content
    QString formattedSnippet(snippet.snippet);
    formattedSnippet.insert(snippet.cursorPosition, QStringLiteral("$|"));
    ui->snippetTextEdit->setPlainText(formattedSnippet);
    ui->snippetTextEdit->setReadOnly(snippet.builtIn);

    // disable remove button when built-in snippet is selected
    ui->removeSnippetButton->setEnabled(!snippet.builtIn);
}

void OptionsDialog::snippetTextChanged()
{
    const QModelIndex &modelIndex = ui->snippetTableView->selectionModel()->currentIndex();
    if (modelIndex.isValid()) {
        Snippet snippet = snippetCollection->at(modelIndex.row());
        if (!snippet.builtIn) {
            snippet.snippet = ui->snippetTextEdit->toPlainText();

            // find cursor marker
            int pos = snippet.snippet.indexOf(QStringLiteral("$|"));
            if (pos >= 0) {
                snippet.cursorPosition = pos;
                snippet.snippet.remove(pos, 2);
            }

            snippetCollection->update(snippet);
        }
    }
}

void OptionsDialog::addSnippetButtonClicked()
{
    SnippetsTableModel *snippetModel = qobject_cast<SnippetsTableModel*>(ui->snippetTableView->model());

    const QModelIndex &index = snippetModel->createSnippet();

    const int row = index.row();
    QModelIndex topLeft = snippetModel->index(row, 0, QModelIndex());
    QModelIndex bottomRight = snippetModel->index(row, 1, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    ui->snippetTableView->selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    ui->snippetTableView->setCurrentIndex(topLeft);
    ui->snippetTableView->scrollTo(topLeft);

    ui->snippetTableView->edit(index);
}

void OptionsDialog::removeSnippetButtonClicked()
{
    const QModelIndex &modelIndex = ui->snippetTableView->selectionModel()->currentIndex();
    if (!modelIndex.isValid()) {
        QMessageBox::critical(0, tr("Error", "Title of error message box"), tr("No snippet selected."));
        return;
    }

    SnippetsTableModel *snippetModel = qobject_cast<SnippetsTableModel*>(ui->snippetTableView->model());
    snippetModel->removeSnippet(modelIndex);
}

void OptionsDialog::validateShortcut(int row, int column)
{
    // Check changes to shortcut column only
    if (column != 1)
        return;

    QString newShortcut = ui->shortcutsTable->item(row, column)->text();
    QKeySequence ks(newShortcut);
    if (ks.isEmpty() && !newShortcut.isEmpty()) {
        // If new shortcut was invalid, restore the original
        ui->shortcutsTable->setItem(row, column,
            new QTableWidgetItem(actions.at(row)->shortcut().toString()));
    } else {
        // Check for conflicts.
        if (!ks.isEmpty()) {
            for (int c = 0; c < actions.size(); ++c) {
                if (c != row && ks == QKeySequence(ui->shortcutsTable->item(c, 1)->text())) {
                    ui->shortcutsTable->setItem(row, column,
                        new QTableWidgetItem(actions.at(row)->shortcut().toString()));
                    QMessageBox::information(this, tr("Conflict"),
                                             tr("This shortcut is already used for \"%1\"")
                                             .arg(actions.at(c)->text().remove(QLatin1Char('&'))));
                    return;
                }
            }
        }
        // If the new shortcut is not the same as the default, make the
        // action label bold.
        QFont font = ui->shortcutsTable->item(row, 0)->font();
        font.setBold(ks != actions.at(row)->property("defaultshortcut").value<QKeySequence>());
        ui->shortcutsTable->item(row, 0)->setFont(font);
    }
}

void OptionsDialog::onPathBrowserButtonClicked()
{
    QFileDialog dialog(this, tr("Choose directory"));
    dialog.setFileMode(QFileDialog::Directory);
    if (dialog.exec() == DialogCode::Accepted) {
        ui->pathLineEdit->setText(dialog.directory().path());
    }
}

void OptionsDialog::setupShortcutsTable()
{
    QStyledItemDelegate *delegate = new QStyledItemDelegate(ui->shortcutsTable);
    QItemEditorFactory *factory = new QItemEditorFactory();
    factory->registerEditor(QVariant::nameToType("QKeySequence"), new KeySequenceEditFactory());
    delegate->setItemEditorFactory(factory);
    ui->shortcutsTable->setItemDelegateForColumn(1, delegate);

    ui->shortcutsTable->setRowCount(actions.size());

    int i = 0;
    for (const QAction *action : qAsConst(actions)) {
        QTableWidgetItem *label = new QTableWidgetItem(action->text().remove('&'));
        label->setFlags(Qt::ItemIsSelectable);
        const QKeySequence &defaultKeySeq = action->property("defaultshortcut").value<QKeySequence>();
        if (action->shortcut() != defaultKeySeq) {
            QFont font = label->font();
            font.setBold(true);
            label->setFont(font);
        }
        QTableWidgetItem *accel = new KeySequenceTableItem(action->shortcut());
        QTableWidgetItem *def = new QTableWidgetItem(defaultKeySeq.toString());
        def->setFlags(Qt::ItemIsSelectable);
        ui->shortcutsTable->setItem(i, 0, label);
        ui->shortcutsTable->setItem(i, 1, accel);
        ui->shortcutsTable->setItem(i, 2, def);
        ++i;
    }

    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    connect(ui->shortcutsTable, &QTableWidget::cellChanged, this, &OptionsDialog::validateShortcut);
}

void OptionsDialog::readState()
{
    // general settings
    ui->converterComboBox->setCurrentIndex(options->markdownConverter());
    ui->pathLineEdit->setText(options->explorerDefaultPath());
    ui->useCurrentFilePathCheckbox->setChecked(options->willUseCurrentFilePath());

    // editor settings
    QFont font = options->editorFont();
    ui->fontComboBox->setCurrentFont(font);
    ui->sizeComboBox->setCurrentText(QString().setNum(font.pointSize()));
    ui->monoFontComboBox->setCurrentFont(options->editorMonoFont());
    ui->sourceSingleSizedCheckBox->setChecked(options->isSourceAtSingleSizeEnabled());
    ui->tabWidthSpinBox->setValue(options->tabWidth());
    ui->lineColumnCheckBox->setChecked(options->isLineColumnEnabled());
    ui->rulerEnableCheckBox->setChecked(options->isRulerEnabled());
    ui->rulerPosSpinBox->setValue(options->rulerPos());

    // html preview settings
    ui->standardFontComboBox->setCurrentFont(options->standardFont());
    ui->defaultSizeComboBox->setCurrentText(QString().setNum(options->defaultFontSize()));
    ui->serifFontComboBox->setCurrentFont(options->serifFont());
    ui->sansSerifFontComboBox->setCurrentFont(options->sansSerifFont());
    ui->fixedFontComboBox->setCurrentFont(options->fixedFont());
    ui->defaultFixedSizeComboBox->setCurrentText(QString().setNum(options->defaultFixedFontSize()));
    ui->mathInlineCheckBox->setChecked(options->isMathInlineSupportEnabled());
    ui->mathSupportCheckBox->setChecked(options->isMathSupportEnabled());

    // proxy settings
    switch (options->proxyMode()) {
    case Options::NoProxy:
        ui->noProxyRadioButton->setChecked(true);
        break;
    case Options::SystemProxy:
        ui->systemProxyRadioButton->setChecked(true);
        break;
    case Options::ManualProxy:
        ui->manualProxyRadioButton->setChecked(true);
        break;
    }
    ui->hostLineEdit->setText(options->proxyHost());
    ui->portLineEdit->setText(QString::number(options->proxyPort()));
    ui->userNameLineEdit->setText(options->proxyUser());
    ui->passwordLineEdit->setText(options->proxyPassword());

    // shortcut settings
    for (int i = 0; i < ui->shortcutsTable->rowCount(); ++i) {
        if (options->hasCustomShortcut(actions.at(i)->objectName())) {
            ui->shortcutsTable->item(i, 1)->setData(Qt::EditRole, options->customShortcut(actions.at(i)->objectName()));
        }
    }
}

void OptionsDialog::saveState()
{
    // general settings
    options->setMarkdownConverter((Options::MarkdownConverter)ui->converterComboBox->currentIndex());
    options->setExplorerDefaultPath(ui->pathLineEdit->text());
    options->setWillUseCurrentFilePath(ui->useCurrentFilePathCheckbox->isChecked());

    // editor settings
    QFont font = ui->fontComboBox->currentFont();
    font.setPointSize(ui->sizeComboBox->currentText().toInt());
    options->setEditorFont(font);
    QFont monoFont = ui->monoFontComboBox->currentFont();
    monoFont.setPointSize(ui->sizeComboBox->currentText().toInt());
    options->setEditorMonoFont(monoFont);
    options->setSourceAtSingleSizeEnabled(ui->sourceSingleSizedCheckBox->isChecked());
    options->setTabWidth(ui->tabWidthSpinBox->value());
    options->setLineColumnEnabled(ui->lineColumnCheckBox->isChecked());
    options->setRulerEnabled(ui->rulerEnableCheckBox->isChecked());
    options->setRulerPos(ui->rulerPosSpinBox->value());
    options->setMathInlineSupportEnabled(ui->mathInlineCheckBox->isChecked());
    options->setMathSupportEnabled(ui->mathSupportCheckBox->isChecked());

    // html preview settings
    options->setStandardFont(ui->standardFontComboBox->currentFont());
    options->setDefaultFontSize(ui->defaultSizeComboBox->currentText().toInt());
    options->setSerifFont(ui->serifFontComboBox->currentFont());
    options->setSansSerifFont(ui->sansSerifFontComboBox->currentFont());
    options->setFixedFont(ui->fixedFontComboBox->currentFont());
    options->setDefaultFixedFontSize(ui->defaultFixedSizeComboBox->currentText().toInt());

    // proxy settings
    if (ui->noProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::NoProxy);
    } else if (ui->systemProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::SystemProxy);
    } else if (ui->manualProxyRadioButton->isChecked()) {
        options->setProxyMode(Options::ManualProxy);
    }
    options->setProxyHost(ui->hostLineEdit->text());
    options->setProxyPort(ui->portLineEdit->text().toInt());
    options->setProxyUser(ui->userNameLineEdit->text());
    options->setProxyPassword(ui->passwordLineEdit->text());

    // shortcut settings
    for (int i = 0; i < ui->shortcutsTable->rowCount(); ++i) {
        QKeySequence customKeySeq(ui->shortcutsTable->item(i, 1)->text());
        options->addCustomShortcut(actions.at(i)->objectName(), customKeySeq);
    }
    
    options->apply();
}

