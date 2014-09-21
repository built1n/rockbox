#include "utils.h"
#include <QFontMetrics>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QHeaderView>
#include <QDebug>
#include <QElapsedTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTextBlock>

/**
 * SocBitRangeValidator
 */
SocBitRangeValidator::SocBitRangeValidator(QObject *parent)
    :QValidator(parent)
{
}

void SocBitRangeValidator::fixup(QString& input) const
{
    input = input.trimmed();
}

QValidator::State SocBitRangeValidator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    int first, last;
    State state = parse(input, last, first);
    return state;
}

QValidator::State SocBitRangeValidator::parse(const QString& input, int& last, int& first) const
{
    // the empty string is always intermediate
    if(input.size() == 0)
        return Intermediate;
    // check if there is ':'
    int pos = input.indexOf(':');
    if(pos == -1)
        pos = input.size();
    // if field start with ':', the last bit is implicit and is 31
    if(pos > 0)
    {
        // parse last bit and check it's between 0 and 31
        bool ok = false;
        last = input.left(pos).toInt(&ok);
        if(!ok || last < 0 || last >= 32)
            return Invalid;
    }
    else
        last = 31;
    // parse first bit
    if(pos < input.size() - 1)
    {
        bool ok = false;
        first = input.mid(pos + 1).toInt(&ok);
        if(!ok || first < 0 || first > last)
            return Invalid;
    }
    // if input ends with ':', first bit is implicit and is 0
    else if(pos == input.size() - 1)
        first = 0;
    // if there no ':', first=last
    else
        first = last;
    return Acceptable;
}

/**
 * SocFieldValidator
 */

SocFieldValidator::SocFieldValidator(QObject *parent)
    :QValidator(parent)
{
    m_field.first_bit = 0;
    m_field.last_bit = 31;
}

SocFieldValidator::SocFieldValidator(const soc_reg_field_t& field, QObject *parent)
    :QValidator(parent), m_field(field)
{
}

void SocFieldValidator::fixup(QString& input) const
{
    input = input.trimmed();
}

QValidator::State SocFieldValidator::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);
    soc_word_t val;
    State state = parse(input, val);
    return state;
}

QValidator::State SocFieldValidator::parse(const QString& input, soc_word_t& val) const
{
    // the empty string is always intermediate
    if(input.size() == 0)
        return Intermediate;
    // first check named values
    State state = Invalid;
    foreach(const soc_reg_field_value_t& value, m_field.value)
    {
        QString name = QString::fromLocal8Bit(value.name.c_str());
        // cannot be a substring if too long or empty
        if(input.size() > name.size())
            continue;
        // check equal string
        if(input == name)
        {
            state = Acceptable;
            val = value.value;
            break;
        }
        // check substring
        if(name.startsWith(input))
            state = Intermediate;
    }
    // early return for exact match
    if(state == Acceptable)
        return state;
    // do a few special cases for convenience
    if(input.compare("0x", Qt::CaseInsensitive) == 0 ||
            input.compare("0b", Qt::CaseInsensitive) == 0)
        return Intermediate;
    // try by parsing
    unsigned basis, pos;
    if(input.size() >= 2 && input.startsWith("0x", Qt::CaseInsensitive))
    {
        basis = 16;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0b", Qt::CaseInsensitive))
    {
        basis = 2;
        pos = 2;
    }
    else if(input.size() >= 2 && input.startsWith("0"))
    {
        basis = 8;
        pos = 1;
    }
    else
    {
        basis = 10;
        pos = 0;
    }
    bool ok = false;
    unsigned long v = input.mid(pos).toULong(&ok, basis);
    // if not ok, return result of name parsing
    if(!ok)
        return state;
    // if ok, check if it fits in the number of bits
    unsigned nr_bits = m_field.last_bit - m_field.first_bit + 1;
    unsigned long max = nr_bits == 32 ? 0xffffffff : (1 << nr_bits) - 1;
    if(v <= max)
    {
        val = v;
        return Acceptable;
    }

    return state;
}

/**
 * RegLineEdit
 */
RegLineEdit::RegLineEdit(QWidget *parent)
    :QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_button = new QToolButton(this);
    m_button->setCursor(Qt::ArrowCursor);
    m_button->setStyleSheet("QToolButton { font-weight: bold; color: white; background: black; }");
    m_button->setPopupMode(QToolButton::InstantPopup);
    m_edit = new QLineEdit(this);
    m_layout->addWidget(m_button);
    m_layout->addWidget(m_edit);
    m_menu = new QMenu(this);
    connect(m_menu->addAction("Write"), SIGNAL(triggered()), this, SLOT(OnWriteAct()));
    connect(m_menu->addAction("Set"), SIGNAL(triggered()), this, SLOT(OnSetAct()));
    connect(m_menu->addAction("Clear"), SIGNAL(triggered()), this, SLOT(OnClearAct()));
    connect(m_menu->addAction("Toggle"), SIGNAL(triggered()), this, SLOT(OnToggleAct()));
    EnableSCT(false);
    SetReadOnly(false);
    ShowMode(true);
    SetMode(Write);
}

void RegLineEdit::SetReadOnly(bool ro)
{
    m_edit->setReadOnly(ro);
    m_readonly = ro;
    ShowMode(!ro);
}

void RegLineEdit::EnableSCT(bool en)
{
    m_has_sct = en;
    if(!m_has_sct)
    {
        m_button->setMenu(0);
        SetMode(Write);
    }
    else
        m_button->setMenu(m_menu);
}

RegLineEdit::~RegLineEdit()
{
}

QLineEdit *RegLineEdit::GetLineEdit()
{
    return m_edit;
}

void RegLineEdit::ShowMode(bool show)
{
    if(show)
        m_button->show();
    else
        m_button->hide();
}

void RegLineEdit::OnWriteAct()
{
    SetMode(Write);
}

void RegLineEdit::OnSetAct()
{
    SetMode(Set);
}

void RegLineEdit::OnClearAct()
{
    SetMode(Clear);
}

void RegLineEdit::OnToggleAct()
{
    SetMode(Toggle);
}

void RegLineEdit::SetMode(EditMode mode)
{
    m_mode = mode;
    switch(m_mode)
    {
        case Write: m_button->setText("WR"); break;
        case Set: m_button->setText("SET"); break;
        case Clear: m_button->setText("CLR"); break;
        case Toggle: m_button->setText("TOG"); break;
        default: break;
    }
}

RegLineEdit::EditMode RegLineEdit::GetMode()
{
    return m_mode;
}

void RegLineEdit::setText(const QString& text)
{
    m_edit->setText(text);
}

QString RegLineEdit::text() const
{
    return m_edit->text();
}

/**
 * SocFieldItemDelegate
 */

QString SocFieldItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    if(value.type() == QVariant::UInt)
        return QString("0x%1").arg(value.toUInt(), (m_bitcount + 3) / 4, 16, QChar('0'));
    else
        return QStyledItemDelegate::displayText(value, locale);
}

/**
 * SocFieldEditor
 */
SocFieldEditor::SocFieldEditor(const soc_reg_field_t& field, QWidget *parent)
    :QLineEdit(parent), m_reg_field(field)
{
    m_validator = new SocFieldValidator(field);
    setValidator(m_validator);
}

SocFieldEditor::~SocFieldEditor()
{
    delete m_validator;
}

uint SocFieldEditor::field() const
{
    soc_word_t v;
    /* in case validator fails to parse, return old value */
    if(m_validator->parse(text(), v) == QValidator::Acceptable)
        return v;
    else
        return m_field;
}

void SocFieldEditor::setField(uint field)
{
    m_field = field;
    int digits = (m_reg_field.last_bit - m_reg_field.first_bit + 4) / 4;
    setText(QString("0x%1").arg(field, digits, 16, QChar('0')));
}

/**
 * SocFieldCachedItemDelegate
 */

QString SocFieldCachedItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    // FIXME see QTBUG-30392
    if(value.type() == QVariant::UserType && value.userType() == qMetaTypeId< SocFieldCachedValue >())
    {
        const SocFieldCachedValue& v = value.value< SocFieldCachedValue >();
        int bitcount = v.field().last_bit - v.field().first_bit;
        return QString("0x%1").arg(v.value(), (bitcount + 3) / 4, 16, QChar('0'));
    }
    else
        return QStyledItemDelegate::displayText(value, locale);
}

/**
 * SocFieldCachedEditor
 */
SocFieldCachedEditor::SocFieldCachedEditor(QWidget *parent)
    :SocFieldEditor(soc_reg_field_t(), parent)
{
}

SocFieldCachedEditor::~SocFieldCachedEditor()
{
}

SocFieldCachedValue SocFieldCachedEditor::value() const
{
    return SocFieldCachedValue(m_value.field(), field());
}

void SocFieldCachedEditor::setValue(SocFieldCachedValue val)
{
    m_value = val;
    SetRegField(m_value.field());
    setField(m_value.value());
}

/**
 * SocFieldEditorCreator
 */
QWidget *SocFieldEditorCreator::createWidget(QWidget *parent) const
{
    return new SocFieldEditor(m_field, parent);
}

QByteArray SocFieldEditorCreator::valuePropertyName() const
{
    return QByteArray("field");
}

/**
 * SocFieldCachedEditorCreator
 */
QWidget *SocFieldCachedEditorCreator::createWidget(QWidget *parent) const
{
    return new SocFieldCachedEditor(parent);
}

QByteArray SocFieldCachedEditorCreator::valuePropertyName() const
{
    return QByteArray("value");
}

/**
 * RegSexyDisplay
 */
RegSexyDisplay::RegSexyDisplay(const SocRegRef& reg, QWidget *parent)
    :QWidget(parent), m_reg(reg)
{
    m_size = QSize();
}

int RegSexyDisplay::separatorSize() const
{
    return 1;
}

int RegSexyDisplay::marginSize() const
{
    return fontMetrics().height() / 3;
}

int RegSexyDisplay::textSep() const
{
    return marginSize() / 2;
}

int RegSexyDisplay::headerHeight() const
{
    return 2 * marginSize() + textSep() + 2 * fontMetrics().height();
}

int RegSexyDisplay::columnWidth() const
{
    return 2 * marginSize() + fontMetrics().height();
}

int RegSexyDisplay::maxContentHeight() const
{
    int max = 0;
    QFontMetrics metrics = fontMetrics();
    for(size_t i = 0; i < m_reg.GetReg().field.size(); i++)
    {
        QString s = QString::fromStdString(m_reg.GetReg().field[i].name);
        // add extra spaces arounds
        s = " " + s + " ";
        max = qMax(max, metrics.boundingRect(s).width());
    }
    return 2 * marginSize() + max;
}

int RegSexyDisplay::gapHeight() const
{
    return marginSize() / 2;
}

QSize RegSexyDisplay::minimumSizeHint() const
{
    /* cache computation because it's expensive */
    if(m_size.isValid())
        return m_size;
    /* width: display 32 columns + 33 vertical separators */
    m_size.setWidth(32 * columnWidth() + 33 * separatorSize());
    /* height: one separator + two digits + one separator + margin + separator
     * + names + separator */
    m_size.setHeight(4 * separatorSize() + headerHeight() + gapHeight() + maxContentHeight());
    return m_size;
}

QSize RegSexyDisplay::sizeHint() const
{
    return minimumSizeHint();
}

void RegSexyDisplay::paintEvent(QPaintEvent *event)
{
    // FIXME could be optimised with QStaticText
    Q_UNUSED(event);
    int txt_h = fontMetrics().height();
    int sep_sz = separatorSize();
    int w = width();
    int h = height() - 1;
    int col_w = (w - 33 * sep_sz) / 32;
    int hdr_h = headerHeight();
    int gap_h = gapHeight();
    int tot_w = 33 * sep_sz + 32 * col_w;
    int margin = marginSize();
    int txt_sep = textSep();
    int tot_hdr_sz = 2 * sep_sz + hdr_h;
    // computer xshift
    int x_shift = (w - tot_w) / 2;
#define ith_col_x(i) (x_shift + (i) * (sep_sz + col_w))

    QPainter painter(this);
    QBrush back_brush = palette().base();
    QBrush line_brush = palette().dark();

    // fill interesting zone with base
    painter.fillRect(x_shift, 0, tot_w, h, back_brush);

    // draw top and bottom lines
    painter.setPen(QPen(palette().dark(), sep_sz));
    painter.fillRect(x_shift, 0, tot_w, sep_sz, line_brush);
    painter.fillRect(x_shift, h - sep_sz, tot_w, sep_sz, line_brush);
    // draw intemediate lines
    for(int i = 0; i <= 32; i++)
        painter.fillRect(ith_col_x(i), 0, sep_sz, 2 * sep_sz + hdr_h, line_brush);
    // draw bottom header lines
    painter.fillRect(ith_col_x(0), sep_sz + hdr_h, tot_w, sep_sz, line_brush);
    painter.fillRect(ith_col_x(0), tot_hdr_sz + gap_h, tot_w, sep_sz, line_brush);
    // redraw some lines but wider
    for(int i = 4; i < 32; i += 4)
        painter.fillRect(ith_col_x(i) - sep_sz, 0, 3 * sep_sz, tot_hdr_sz, line_brush);
    // draw numbers in the header
    painter.setPen(palette().brush(QPalette::ButtonText).color());
    for(int i = 0; i < 32; i++)
    {
        QRect r(ith_col_x(i), sep_sz + margin, col_w, txt_h);
        painter.drawText(r, Qt::AlignCenter, QString("%1").arg((31 - i) / 10));
        r.translate(0, txt_h + txt_sep);
        painter.drawText(r, Qt::AlignCenter, QString("%1").arg((31 - i) % 10));
    }
    // display content
    for(size_t i = 0; i < m_reg.GetReg().field.size(); i++)
    {
        const soc_reg_field_t& field = m_reg.GetReg().field[i];
        QRect r(QPoint(ith_col_x(31 - field.last_bit) + sep_sz, tot_hdr_sz),
            QPoint(ith_col_x(32 - field.first_bit), h - sep_sz));
        painter.fillRect(r.x() - sep_sz, r.y(), sep_sz, r.height(), line_brush);
        painter.fillRect(r.right(), r.y(), sep_sz, r.height(), line_brush);
        r.setY(r.y() + gap_h + sep_sz);
        // draw rotated text
        painter.save();
        painter.translate(r.bottomLeft());
        painter.rotate(-90);
        //painter.fillRect(QRect(0, 0, r.height(), r.width()), QBrush(Qt::red));
        QRect r2(0, 0, r.height(), r.width());
        painter.drawText(r2, Qt::AlignCenter, QString::fromStdString(field.name));
        painter.restore();
    }
#undef ith_col_x
}

/**
 * GrowingTextEdit
 */
GrowingTextEdit::GrowingTextEdit(QWidget *parent)
    :QTextEdit(parent)
{
    connect(this, SIGNAL(textChanged()), this, SLOT(TextChanged()));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void GrowingTextEdit::TextChanged()
{
    int content_size = document()->documentLayout()->documentSize().height();
    content_size = qMax(content_size, fontMetrics().height());
    setFixedHeight(content_size + contentsMargins().top() + contentsMargins().bottom());
}

/**
 * GrowingTableWidget
 */
GrowingTableWidget::GrowingTableWidget(QWidget *parent)
    :QTableWidget(parent)
{
    connect(model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(DataChanged(const QModelIndex&, const QModelIndex&)));
}

void GrowingTableWidget::DataChanged(const QModelIndex& tl, const QModelIndex& br)
{
    Q_UNUSED(tl);
    Q_UNUSED(br);
    resizeRowsToContents();
    resizeColumnsToContents();
    int h = contentsMargins().top() + contentsMargins().bottom();
    h += horizontalHeader()->height();
    for(int i = 0; i < rowCount(); i++)
        h += rowHeight(i);
    setMinimumHeight(h);
}

/**
 * MyTextEditor
 */
MyTextEditor::MyTextEditor(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_toolbar = new QToolBar(this);
    m_edit = new QTextEdit(this);
    layout->addWidget(m_toolbar, 0);
    layout->addWidget(m_edit, 1);
    setLayout(layout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_edit->setAcceptRichText(false);
    m_edit->setAutoFormatting(QTextEdit::AutoAll);

    m_bold_button = new QToolButton(this);
    m_bold_button->setIcon(QIcon::fromTheme("format-text-bold"));
    m_bold_button->setText("bold");
    m_bold_button->setCheckable(true);

    m_italic_button = new QToolButton(this);
    m_italic_button->setIcon(QIcon::fromTheme("format-text-italic"));
    m_italic_button->setText("italic");
    m_italic_button->setCheckable(true);

    m_underline_button = new QToolButton(this);
    m_underline_button->setIcon(QIcon::fromTheme("format-text-underline"));
    m_underline_button->setText("underline");
    m_underline_button->setCheckable(true);

    m_toolbar->addWidget(m_bold_button);
    m_toolbar->addWidget(m_italic_button);
    m_toolbar->addWidget(m_underline_button);

    connect(m_bold_button, SIGNAL(toggled(bool)), this, SLOT(OnTextBold(bool)));
    connect(m_italic_button, SIGNAL(toggled(bool)), this, SLOT(OnTextItalic(bool)));
    connect(m_underline_button, SIGNAL(toggled(bool)), this, SLOT(OnTextUnderline(bool)));
    connect(m_edit, SIGNAL(textChanged()), this, SLOT(OnInternalTextChanged()));
    connect(m_edit, SIGNAL(currentCharFormatChanged(const QTextCharFormat&)),
        this, SLOT(OnCharFormatChanged(const QTextCharFormat&)));

    SetGrowingMode(false);
    SetReadOnly(false);
}

void MyTextEditor::SetReadOnly(bool en)
{
    m_read_only = en;
    if(en)
        m_toolbar->hide();
    else
        m_toolbar->hide();
    m_edit->setReadOnly(en);
}

void MyTextEditor::SetGrowingMode(bool en)
{
    m_growing_mode = en;
    if(en)
    {
        m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        OnTextChanged();
    }
    else
    {
        m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void MyTextEditor::OnInternalTextChanged()
{
    if(m_growing_mode)
    {
        int content_size = m_edit->document()->documentLayout()->documentSize().height();
        content_size = qMax(content_size, m_edit->fontMetrics().height());
        m_edit->setMinimumHeight(content_size + m_edit->contentsMargins().top() + 
            m_edit->contentsMargins().bottom());
    }
    emit OnTextChanged();
}

void MyTextEditor::OnTextBold(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnTextItalic(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontItalic(checked);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnTextUnderline(bool checked)
{
    QTextCursor cursor = m_edit->textCursor();
    QTextCharFormat fmt = cursor.charFormat();
    fmt.setFontUnderline(checked);
    cursor.setCharFormat(fmt);
    m_edit->setTextCursor(cursor);
}

void MyTextEditor::OnCharFormatChanged(const QTextCharFormat& fmt)
{
    /* NOTE: changing the button states programmaticaly doesn't trigger
     * the toggled() signals, otherwise it would result in a loop
     * between this function and OnText{Bold,Italic,Underline,...} */
    m_bold_button->setChecked(fmt.fontWeight() > QFont::Normal);
    m_italic_button->setChecked(fmt.fontItalic());
    m_underline_button->setChecked(fmt.fontUnderline());
}

void MyTextEditor::SetTextHtml(const QString& text)
{
    m_edit->setHtml(text);
}

QString MyTextEditor::GetTextHtml()
{
    return m_edit->toPlainText();
}

bool MyTextEditor::IsModified()
{
    return m_edit->document()->isModified();
}

/**
 * MySwitchableTextEditor
 */
MySwitchableTextEditor::MySwitchableTextEditor(QWidget *parent)
    :QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_edit = new MyTextEditor(this);
    m_label = new QLabel(this);
    m_label->setTextFormat(Qt::RichText);
    m_label->setAlignment(Qt::AlignTop);
    m_line = new QLineEdit(this);

    layout->addWidget(m_label);
    layout->addWidget(m_edit);
    layout->addWidget(m_line);

    setLayout(layout);

    m_editor_mode = false;
    m_line_mode = false;
    UpdateVisibility();
}

void MySwitchableTextEditor::SetEditorMode(bool edit)
{
    if(edit == m_editor_mode)
        return;
    QString text = GetTextHtml();
    m_editor_mode = edit;
    UpdateVisibility();
    SetTextHtml(text);
}

QString MySwitchableTextEditor::GetTextHtml()
{
    if(m_editor_mode)
        return m_line_mode ? m_line->text() : m_edit->GetTextHtml();
    else
        return m_label->text();
}

void MySwitchableTextEditor::SetTextHtml(const QString& text)
{
    if(m_editor_mode)
    {
        if(m_line_mode)
            m_line->setText(text);
        else
            m_edit->SetTextHtml(text);
    }
    else
        m_label->setText(text);
}

MyTextEditor *MySwitchableTextEditor::GetEditor()
{
    return m_edit;
}

void MySwitchableTextEditor::SetLineMode(bool en)
{
    if(m_line_mode == en)
        return;
    QString text = GetTextHtml();
    m_line_mode = en;
    SetTextHtml(text);
    UpdateVisibility();
}

QLineEdit *MySwitchableTextEditor::GetLineEdit()
{
    return m_line;
}

void MySwitchableTextEditor::UpdateVisibility()
{
    m_label->setVisible(!m_editor_mode);
    m_edit->setVisible(m_editor_mode && !m_line_mode);
    m_line->setVisible(m_editor_mode && m_line_mode);
}

QLabel *MySwitchableTextEditor::GetLabel()
{
    return m_label;
}

bool MySwitchableTextEditor::IsModified()
{
    if(!m_editor_mode)
        return false;
    return m_line_mode ? m_line->isModified() : m_edit->IsModified();
}

/**
 * BackendSelector
 */
BackendSelector::BackendSelector(Backend *backend, QWidget *parent)
    :QWidget(parent), m_backend(backend)
{
    m_data_selector = new QComboBox;
    m_data_selector->addItem(QIcon::fromTheme("text-x-generic"), "Nothing...", QVariant(DataSelNothing));
    m_data_selector->addItem(QIcon::fromTheme("document-open"), "File...", QVariant(DataSelFile));
#ifdef HAVE_HWSTUB
    m_data_selector->addItem(QIcon::fromTheme("multimedia-player"), "Device...", QVariant(DataSelDevice));
#endif
    m_data_sel_edit = new QLineEdit;
    m_data_sel_edit->setReadOnly(true);
    QHBoxLayout *data_sel_layout = new QHBoxLayout(this);
    data_sel_layout->addWidget(m_data_selector);
    data_sel_layout->addWidget(m_data_sel_edit, 1);
    data_sel_layout->addStretch(0);
#ifdef HAVE_HWSTUB
    m_dev_selector = new QComboBox;
    data_sel_layout->addWidget(m_dev_selector, 1);
#endif

    m_io_backend = m_backend->CreateDummyIoBackend();

    connect(m_data_selector, SIGNAL(activated(int)),
        this, SLOT(OnDataSelChanged(int)));
#ifdef HAVE_HWSTUB
    connect(m_dev_selector, SIGNAL(currentIndexChanged(int)),
        this, SLOT(OnDevChanged(int)));
    connect(&m_hwstub_helper, SIGNAL(OnDevListChanged(bool, struct libusb_device *)),
        this, SLOT(OnDevListChanged2(bool, struct libusb_device *)));
#endif
    OnDataSelChanged(0);
}

BackendSelector::~BackendSelector()
{
#ifdef HAVE_HWSTUB
    ClearDevList();
#endif
    delete m_io_backend;
}

void BackendSelector::OnDataSelChanged(int index)
{
    if(index == -1)
        return;
    QVariant var = m_data_selector->itemData(index);
    if(var == DataSelFile)
    {
        m_data_sel_edit->show();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif
        QFileDialog *fd = new QFileDialog(m_data_selector);
        fd->setFilter("Textual files (*.txt);;All files (*)");
        fd->setDirectory(Settings::Get()->value("loaddatadir", QDir::currentPath()).toString());
        if(fd->exec())
        {
            QStringList filenames = fd->selectedFiles();
            ChangeBackend(m_backend->CreateFileIoBackend(filenames[0]));
            m_data_sel_edit->setText(filenames[0]);
        }
        Settings::Get()->setValue("loaddatadir", fd->directory().absolutePath());
    }
#ifdef HAVE_HWSTUB
    else if(var == DataSelDevice)
    {
        m_data_sel_edit->hide();;
        m_dev_selector->show();
        OnDevListChanged();
    }
#endif
    else
    {
        m_data_sel_edit->hide();
#ifdef HAVE_HWSTUB
        m_dev_selector->hide();
#endif

        ChangeBackend(m_backend->CreateDummyIoBackend());
    }
}

#ifdef HAVE_HWSTUB
void BackendSelector::OnDevListChanged2(bool arrived, struct libusb_device *dev)
{
    Q_UNUSED(arrived);
    Q_UNUSED(dev);
    OnDevListChanged();
}

void BackendSelector::OnDevListChanged()
{
    ClearDevList();
    QList< HWStubDevice* > list = m_hwstub_helper.GetDevList();
    foreach(HWStubDevice *dev, list)
    {
        QString name = QString("Bus %1 Device %2: %3").arg(dev->GetBusNumber())
            .arg(dev->GetDevAddress()).arg(dev->GetTargetInfo().bName);
        m_dev_selector->addItem(QIcon::fromTheme("multimedia-player"), name,
            QVariant::fromValue((void *)dev));
    }
    if(list.size() > 0)
        m_dev_selector->setCurrentIndex(0);
}

void BackendSelector::OnDevChanged(int index)
{
    if(index == -1)
        return;
    HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(index).value< void* >());
    delete m_io_backend;
    /* NOTE: make a copy of the HWStubDevice device because the one in the list
     * might get destroyed when clearing the list while the backend is still
     * active: this would result in a double free when the backend is also destroyed */
    m_io_backend = m_backend->CreateHWStubIoBackend(new HWStubDevice(dev));
    emit OnSelect(m_io_backend);
}

void BackendSelector::ClearDevList()
{
    while(m_dev_selector->count() > 0)
    {
        HWStubDevice *dev = reinterpret_cast< HWStubDevice* >(m_dev_selector->itemData(0).value< void* >());
        delete dev;
        m_dev_selector->removeItem(0);
    }
}
#endif

IoBackend *BackendSelector::GetBackend()
{
    return m_io_backend;
}

void BackendSelector::ChangeBackend(IoBackend *new_backend)
{
    /* WARNING: delete old backend *after* calling the signal, otherwise the old backend
     * might get used after delete */
    emit OnSelect(new_backend);
    delete m_io_backend;
    m_io_backend = new_backend;
}
