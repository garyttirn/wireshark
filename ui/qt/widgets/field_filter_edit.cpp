/* field_filter_edit.cpp
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/proto.h>

#include <wsutil/utf8_entities.h>

#include <ui/qt/widgets/field_filter_edit.h>
#include "filter_dialog.h"
#include <ui/qt/widgets/stock_icon_tool_button.h>
#include <ui/qt/widgets/syntax_line_edit.h>

#include <QAction>
#include <QCompleter>
#include <QEvent>
#include <QStringListModel>

#include <wsutil/utf8_entities.h>

#ifdef __APPLE__
#define DEFAULT_MODIFIER UTF8_PLACE_OF_INTEREST_SIGN
#else
#define DEFAULT_MODIFIER "Ctrl-"
#endif

// proto.c:fld_abbrev_chars
static const QString fld_abbrev_chars_ = "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

FieldFilterEdit::FieldFilterEdit(QWidget *parent) :
    SyntaxLineEdit(parent)
{
    setAccessibleName(tr("Field entry"));

    completion_model_ = new QStringListModel(this);
    setCompleter(new QCompleter(completion_model_, this));
    setCompletionTokenChars(fld_abbrev_chars_);

    setDefaultPlaceholderText();

    connect(this, &FieldFilterEdit::textChanged, this,
            static_cast<void (FieldFilterEdit::*)(const QString &)>(&FieldFilterEdit::checkFilter));
}

void FieldFilterEdit::setDefaultPlaceholderText()
{
    placeholder_text_ = tr("Enter a field %1").arg(UTF8_HORIZONTAL_ELLIPSIS);

    setPlaceholderText(placeholder_text_);
}

void FieldFilterEdit::focusOutEvent(QFocusEvent *event)
{
    if (syntaxState() == Valid)
        emit popFilterSyntaxStatus();
    SyntaxLineEdit::focusOutEvent(event);
}

bool FieldFilterEdit::checkFilter()
{
    checkFilter(text());

    return syntaxState() != Invalid;
}

void FieldFilterEdit::checkFilter(const QString& filter_text)
{
    popFilterSyntaxStatus();

    QString field_word = filter_text.trimmed();

    if (field_word.isEmpty()) {
        setSyntaxState(SyntaxLineEdit::Empty);
    } else {
        const header_field_info *hfinfo = proto_registrar_get_byname(field_word.toUtf8().constData());
        if (hfinfo) {
            setSyntaxState(SyntaxLineEdit::Valid);
        } else {
            hfinfo = proto_registrar_get_byalias(field_word.toUtf8().constData());
            if (hfinfo) {
                setSyntaxState(SyntaxLineEdit::Deprecated);
            } else {
                setSyntaxState(SyntaxLineEdit::Invalid);
            }
        }
    }

    switch (syntaxState()) {
    case Deprecated:
    {
        emit pushFilterSyntaxWarning(syntaxErrorMessage());
        break;
    }
    case Invalid:
    {
        QString invalidMsg(tr("Invalid field: "));
        invalidMsg.append(syntaxErrorMessage());
        emit pushFilterSyntaxStatus(invalidMsg);
        break;
    }
    default:
        break;
    }
}

void FieldFilterEdit::buildCompletionList(const QString &field_word, const QString &preamble)
{
    // Push a hint about the current field.
    if (syntaxState() == Valid) {
        emit popFilterSyntaxStatus();

        header_field_info *hfinfo = proto_registrar_get_byname(field_word.toUtf8().constData());
        if (hfinfo) {
            QString cursor_field_msg = QStringLiteral("%1: %2")
                    .arg(hfinfo->name)
                    .arg(ftype_pretty_name(hfinfo->type));
            emit pushFilterSyntaxStatus(cursor_field_msg);
        }
    }

    // Single fields only; don't offer completion if there's a preamble.
    if (!preamble.isEmpty()) {
        completion_model_->setStringList(QStringList());
        return;
    }

    if (field_word.length() < 1) {
        completion_model_->setStringList(QStringList());
        return;
    }

    void *proto_cookie;
    QStringList field_list;
    int field_dots = static_cast<int>(field_word.count('.')); // Some protocol names (_ws.expert) contain periods.
    for (int proto_id = proto_get_first_protocol(&proto_cookie); proto_id != -1; proto_id = proto_get_next_protocol(&proto_cookie)) {
        protocol_t *protocol = find_protocol_by_id(proto_id);
        if (!proto_is_protocol_enabled(protocol)) continue;

        const QString pfname = proto_get_protocol_filter_name(proto_id);
        field_list << pfname;

        // Add fields only if we're past the protocol name and only for the
        // current protocol.
        if (field_dots > pfname.count('.')) {
            void *field_cookie;
            const QByteArray fw_ba = field_word.toUtf8(); // or toLatin1 or toStdString?
            const char *fw_utf8 = fw_ba.constData();
            size_t fw_len = (size_t) strlen(fw_utf8);
            for (header_field_info *hfinfo = proto_get_first_protocol_field(proto_id, &field_cookie); hfinfo; hfinfo = proto_get_next_protocol_field(proto_id, &field_cookie)) {
                if (hfinfo->same_name_prev_id != -1) continue; // Ignore duplicate names.

                if (!g_ascii_strncasecmp(fw_utf8, hfinfo->abbrev, fw_len)) {
                    if ((size_t) strlen(hfinfo->abbrev) != fw_len) field_list << hfinfo->abbrev;
                }
            }
        }
    }
    field_list.sort();

    completion_model_->setStringList(field_list);
    completer()->setCompletionPrefix(field_word);
}

void FieldFilterEdit::changeEvent(QEvent* event)
{
    if (0 != event)
    {
        switch (event->type())
        {
        case QEvent::LanguageChange:
            setDefaultPlaceholderText();
            break;
        default:
            break;
        }
    }
    SyntaxLineEdit::changeEvent(event);
}
