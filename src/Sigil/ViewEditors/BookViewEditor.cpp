/************************************************************************
**
**  Copyright (C) 2012 John Schember <john@nachtimwald.com>
**
**  This file is part of Sigil.
**
**  Sigil is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Sigil is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Sigil.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>
#include <QtGui/QTextDocument>
#include <QtWebKit/QWebFrame>

#include "BookManipulation/Book.h"
#include "Misc/SettingsStore.h"
#include "Misc/Utility.h"
#include "PCRE/PCRECache.h"
#include "sigil_constants.h"
#include "ViewEditors/BookViewEditor.h"

const int PROGRESS_BAR_MINIMUM_DURATION = 1500;
const QString BV_BREAK_TAG_INSERT = "<hr class=\"sigilBVTmpChapterBreak\" />";
const QString BV_BREAK_TAG_SEARCH  = "(<div>\\s*)?<hr\\s*class\\s*=\\s*\"[^\"]*sigilBVTmpChapterBreak\[^\"]*\"\\s*/>(\\s*</div>)?";

const QString BREAK_TAG_INSERT    = "<hr class=\"sigilChapterBreak\" />";
// %1 = CKE path.
// %2 = Text to load.
// %3 = language
// %4 = customConfig : '/custom/ckeditor_config.js'.
const QString CKE_BASE =
    "<html>"
    "<head>"
    "    <script type=\"text/javascript\" src=\"%1/ckeditor.js\"></script>"
    "</head>"
    "<body>"
    "    <textarea id=\"editor\" name=\"editor\">%2</textarea>"
    "    <script type=\"text/javascript\">"
    "        CKEDITOR.replace('editor', { fullPage: true, startupFocus: true, extraPlugins: 'onchange,docprops', language: '%3',"
    "            coreStyles_bold: { element : 'span', attributes :  {'style': 'font-weight: bold;'} },"
    "            coreStyles_italic: { element : 'span', attributes : {'style': 'font-style: italic;'}},"
    "            coreStyles_underline: { element : 'span', attributes : {'style': 'text-decoration: underline;'}},"
    "            coreStyles_strike: { element : 'span', attributes : {'style': 'text-decoration: line-through;'}, onchangeverrides : 'strike' },"
    "            coreStyles_subscript : { element : 'span', attributes : {'style': 'vertical-align: sub; font-size: smaller;'}, overrides : 'sub' },"
    "            coreStyles_superscript : { elementent : 'span', attributes : {'style': 'vertical-align: super; font-size: smaller;'}, overrides : 'sup' },"
    "        %4});"
    "        CKEDITOR.on('instanceReady', function(e) { e.editor.execCommand('maximize'); });"
    "        CKEDITOR.instances.editor.on('change', function() { BookViewEditor.TextChangedFilter(); });"
    "    </script>"
    "</body>"
    "</html>";

BookViewEditor::BookViewEditor(QWidget *parent)
    :
    BookViewPreview(parent)
{
    page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
    page()->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
}

void BookViewEditor::CustomSetDocument(const QString &path, const QString &html)
{
    m_isLoadFinished = false;
    m_path = path;

    QString cke_path;
#ifdef Q_WS_MAC
    cke_path = QCoreApplication::applicationDirPath() + "/../ckeditor";
#else
    cke_path = QCoreApplication::applicationDirPath() + "/ckeditor";
#endif
// On *nix we have an installation path we need to check too.
#ifdef Q_WS_X11
    if (cke_path.isEmpty() || !QDir(cke_path).exists()) {
        cke_path = QCoreApplication::applicationDirPath() + "/../share/" + QCoreApplication::applicationName().toLower() + "/ckeditor";
    }
#endif

    QString cke_settings;
    QString cke_setting_path = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/ckeditor_config.js";
    if (QFile::exists(cke_setting_path)) {
        cke_settings = QString("customConfig: '%1'").arg(cke_setting_path);
    }

    SettingsStore settings;
    QString base = CKE_BASE.arg(QDir::fromNativeSeparators(cke_path)).arg(Qt::escape(html)).arg(settings.uiLanguage()).arg(cke_settings);
    setHtml(base, QUrl::fromLocalFile(path));
    page()->mainFrame()->addToJavaScriptWindowObject("BookViewEditor", this);
}

void BookViewEditor::ScrollToFragment(const QString &fragment)
{
    if (fragment.isEmpty()) {
        ScrollToTop();
        return;
    }

    QString scroll = "var documentWrapper = CKEDITOR.instances.editor.dom.document;"
        "var element = documentWrapper.getById(\"" % fragment % "\");"
        "element.scrollIntoView()";
    EvaluateJavascript(scroll % SET_CURSOR_JS);
}

void BookViewEditor::ScrollToFragmentAfterLoad(const QString &fragment)
{
    if (fragment.isEmpty()) {
        return;
    }

    QString scroll = "var documentWrapper = CKEDITOR.instances.editor.dom.document;"
        "var element = documentWrapper.getById(\"" % fragment % "\");"
        "element.scrollIntoView()";
    QString javascript = "window.addEventListener('load', GoToFragment, false);"
        "function GoToFragment() { " % scroll % SET_CURSOR_JS % "}";

    EvaluateJavascript(javascript);
}

QString BookViewEditor::GetHtml()
{
    QString command = "CKEDITOR.instances.editor.getData();";
    return EvaluateJavascript(command).toString();
}

QString BookViewEditor::GetXHtml11()
{
    return GetHtml();
}

QString BookViewEditor::GetHtml5()
{
    return GetHtml();
}

void BookViewEditor::InsertHtml(const QString &html)
{
    QString javascript = "CKEDITOR.instances.editor.insertHtml('" + html + "');";
    EvaluateJavascript(javascript);
}

QString BookViewEditor::SplitChapter()
{
    // Add a temporary break marker so we know where the user has requested
    // the text to be split.
    InsertHtml(BV_BREAK_TAG_INSERT);

    QString new_text;
    QString text = GetHtml();

    QRegExp body_search(BODY_START);
    int body_tag_start = text.indexOf(body_search);

    QString head = text.left(body_tag_start);

    QRegExp break_tag(BV_BREAK_TAG_SEARCH);
    int break_index = text.indexOf(break_tag, body_tag_start + body_search.matchedLength());
    if (break_index != -1) {
        // Create a new HTML string with the text that is being split.
        new_text = Utility::Substring(0, break_index, text) + "</body></html>";

        // Remove the the split text from this document and set it
        // as the text in the editor.
        text = head + Utility::Substring(break_index + break_tag.matchedLength(), text.length(), text);
        CustomSetDocument(m_path, text);
    }

    return new_text;
}

//   We need to make sure that the Book View has focus,
// but just calling setFocus isn't enough because Nokia
// did a terrible job integrating Webkit. So we first
// have to steal focus away, and then give it back.
//   If we don't steal focus first, then the QWebView
// can have focus (and its QWebFrame) and still not
// really have it (no blinking cursor).
void BookViewEditor::GrabFocus()
{
    qobject_cast<QWidget *>(parent())->setFocus();
    QWebView::setFocus();
}

bool BookViewEditor::IsModified()
{
    QString javascript = "CKEDITOR.instances.editor.checkDirty();";
    return EvaluateJavascript(javascript).toBool();
}

void BookViewEditor::ResetModified()
{
    QString javascript = "CKEDITOR.instances.editor.resetDirty();";
    EvaluateJavascript(javascript);
}

void BookViewEditor::Undo()
{
    QString javascript = "CKEDITOR.instances.editor.undo();";
    EvaluateJavascript(javascript);
}

void BookViewEditor::Redo()
{
    QString javascript = "CKEDITOR.instances.editor.redo();";
    EvaluateJavascript(javascript);
}

// Overridden so we can emit the FocusLost() signal.
void BookViewEditor::focusOutEvent(QFocusEvent *event)
{
    emit FocusLost(this);
    QWebView::focusOutEvent(event);
}

bool BookViewEditor::FindNext(const QString &search_regex,
                              Searchable::Direction search_direction,
                              bool check_spelling,
                              bool ignore_selection_offset,
                              bool wrap
                             )
{
    Q_UNUSED(check_spelling)
    Q_UNUSED(ignore_selection_offset)

    // We can't handle a regex so remove the regex code.
    QString search_text = search_regex;
    // Remove the case insensitive parameter.
    search_text = search_text.remove(0, 4);
    search_text = search_text.replace(QRegExp("\\([^\\])"), "\1");
    search_text = search_text.replace("\\\\", "\\");

    QWebPage::FindFlags flags;
    if (search_direction == Searchable::Direction_Up) {
        flags  |= QWebPage::FindBackward;
    }
    if (wrap) {
        flags |= QWebPage::FindWrapsAroundDocument;
    }

    return findText(search_text, flags);
}

int BookViewEditor::Count(const QString &search_regex, bool check_spelling)
{
    return 0;
}

bool BookViewEditor::ReplaceSelected(const QString &search_regex, const QString &replacement, Searchable::Direction direction, bool check_spelling)
{
    QMessageBox::critical(this, tr("Unsupported"), tr("Replace is not supported in Book View at this time.  Switch to Code View."));
    return false;
}

int BookViewEditor::ReplaceAll(const QString &search_regex, const QString &replacement, bool check_spelling)
{
    QMessageBox::critical(this, tr("Unsupported"), tr("Replace All for the current file is not supported in Book View at this time.  Switch to Code View."));
    return 0;
}

QString BookViewEditor::GetSelectedText()
{
    QString javascript = "CKEDITOR.instances.editor.getSelection().getSelectedText();";
    return EvaluateJavascript(javascript).toString();
}

void BookViewEditor::TextChangedFilter()
{
    emit textChanged();
}

