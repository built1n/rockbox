#include "regdisplaypanel.h"
#include <QHeaderView>
#include <QDebug>

/**
 * RegItemEditorCreator
 */

QWidget *RegItemEditorCreator::createWidget(QWidget * parent) const
{
    return new RegLineEdit(parent);
}

QByteArray RegItemEditorCreator::valuePropertyName () const
{
    return QByteArray("text");
}

/**
 * SocDisplayPanel
 */
SocDisplayPanel::SocDisplayPanel(QWidget *parent, const SocRef& dev_ref)
    :QGroupBox(parent), m_soc(dev_ref)
{
    QVBoxLayout *right_layout = new QVBoxLayout;

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(m_soc.GetSoc().name) + "</h1>");

    m_desc = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_soc.GetSoc().desc));

    right_layout->addWidget(m_name, 0);
    right_layout->addWidget(m_desc, 0);
    right_layout->addStretch(1);

    setTitle("System-on-Chip Description");
    setLayout(right_layout);
}

void SocDisplayPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *SocDisplayPanel::GetWidget()
{
    return this;
}

/**
 * DevDisplayPanel
 */
DevDisplayPanel::DevDisplayPanel(QWidget *parent, const SocDevRef& dev_ref)
    :QGroupBox(parent), m_dev(dev_ref), m_reg_font(font())
{
    QVBoxLayout *right_layout = new QVBoxLayout;
    const soc_dev_addr_t& dev_addr = m_dev.GetDevAddr();

    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    QString dev_name;
    dev_name.sprintf("HW_%s_BASE", dev_addr.name.c_str());

    QLabel *label_names = new QLabel("<b>" + dev_name + "</b>");
    label_names->setTextFormat(Qt::RichText);

    QLabel *label_addr = new QLabel("<b>" + QString().sprintf("0x%03x", dev_addr.addr) + "</b>");
    label_addr->setTextFormat(Qt::RichText);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    m_name = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_name->setText("<h1>" + QString::fromStdString(m_dev.GetDev().long_name) + "</h1>");

    m_desc = new QLabel(this);
    m_name->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_dev.GetDev().desc));

    right_layout->addWidget(m_name, 0);
    right_layout->addLayout(top_layout, 0);
    right_layout->addWidget(m_desc, 0);
    right_layout->addStretch(1);

    setTitle("Device Description");
    setLayout(right_layout);
}

void DevDisplayPanel::AllowWrite(bool en)
{
    Q_UNUSED(en);
}

QWidget *DevDisplayPanel::GetWidget()
{
    return this;
}

/**
 * RegDisplayPanel
 */

RegDisplayPanel::RegDisplayPanel(QWidget *parent, IoBackend *io_backend, const SocRegRef& reg_ref)
    :QGroupBox(parent), m_io_backend(io_backend), m_reg(reg_ref), m_reg_font(font())
{
    bool read_only = m_io_backend->IsReadOnly();

    QVBoxLayout *right_layout = new QVBoxLayout;

    const soc_dev_addr_t& dev_addr = m_reg.GetDevAddr();
    const soc_reg_t& reg = m_reg.GetReg();
    const soc_reg_addr_t& reg_addr = m_reg.GetRegAddr();

    m_reg_font.setWeight(100);
    m_reg_font.setKerning(false);

    QString reg_name;
    reg_name.sprintf("HW_%s_%s", dev_addr.name.c_str(), reg_addr.name.c_str());
    QStringList names;
    QVector< soc_addr_t > addresses;
    names.append(reg_name);
    addresses.append(reg_addr.addr);
    if(reg.flags & REG_HAS_SCT)
    {
        names.append(reg_name + "_SET");
        names.append(reg_name + "_CLR");
        names.append(reg_name + "_TOG");
        addresses.append(reg_addr.addr + 4);
        addresses.append(reg_addr.addr + 8);
        addresses.append(reg_addr.addr + 12);
    }

    QString str;
    str += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str += "<tr><td><b>" + names[i] + "</b></td></tr>";
    str += "</table>";
    QLabel *label_names = new QLabel;
    label_names->setTextFormat(Qt::RichText);
    label_names->setText(str);

    QString str_addr;
    str_addr += "<table align=left>";
    for(int i = 0; i < names.size(); i++)
        str_addr += "<tr><td><b>" + QString().sprintf("0x%03x", addresses[i]) + "</b></td></tr>";
    str_addr += "</table>";
    QLabel *label_addr = new QLabel;
    label_addr->setTextFormat(Qt::RichText);
    label_addr->setText(str_addr);

    QHBoxLayout *top_layout = new QHBoxLayout;
    top_layout->addStretch();
    top_layout->addWidget(label_names);
    top_layout->addWidget(label_addr);
    top_layout->addStretch();

    m_raw_val_name = new QLabel;
    m_raw_val_name->setText("Raw value:");
    m_raw_val_edit = new RegLineEdit;
    m_raw_val_edit->SetReadOnly(read_only);
    m_raw_val_edit->GetLineEdit()->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_raw_val_edit->GetLineEdit()->setValidator(new SocFieldValidator(m_raw_val_edit));
    m_raw_val_edit->EnableSCT(!!(reg.flags & REG_HAS_SCT));
    m_raw_val_edit->GetLineEdit()->setFont(m_reg_font);
    QHBoxLayout *raw_val_layout = new QHBoxLayout;
    raw_val_layout->addStretch();
    raw_val_layout->addWidget(m_raw_val_name);
    raw_val_layout->addWidget(m_raw_val_edit);
    raw_val_layout->addStretch();

    m_value_table = new GrowingTableWidget;
    m_value_table->setRowCount(reg.field.size());
    m_value_table->setColumnCount(5);
    for(size_t row = 0; row < reg.field.size(); row++)
    {
        const soc_reg_field_t& field = reg.field[row];
        QString bits_str;
        if(field.first_bit == field.last_bit)
            bits_str.sprintf("%d", field.first_bit);
        else
            bits_str.sprintf("%d:%d", field.last_bit, field.first_bit);
        QTableWidgetItem *item = new QTableWidgetItem(bits_str);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_value_table->setItem(row, FieldBitsColumn, item);
        item = new QTableWidgetItem(QString(field.name.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_value_table->setItem(row, FieldNameColumn, item);
        item = new QTableWidgetItem();
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(Qt::DisplayRole, QVariant::fromValue(SocFieldCachedValue()));
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        m_value_table->setItem(row, FieldValueColumn, item);
        item = new QTableWidgetItem("");
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        m_value_table->setItem(row, FieldMeaningColumn, item);
        item = new QTableWidgetItem(QString(field.desc.c_str()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_value_table->setItem(row, FieldDescColumn, item);
    }
    m_value_table->setHorizontalHeaderItem(FieldBitsColumn, new QTableWidgetItem("Bits"));
    m_value_table->setHorizontalHeaderItem(FieldNameColumn, new QTableWidgetItem("Name"));
    m_value_table->setHorizontalHeaderItem(FieldValueColumn, new QTableWidgetItem("Value"));
    m_value_table->setHorizontalHeaderItem(FieldMeaningColumn, new QTableWidgetItem("Meaning"));
    m_value_table->setHorizontalHeaderItem(FieldDescColumn, new QTableWidgetItem("Description"));
    m_value_table->verticalHeader()->setVisible(false);
    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
    m_value_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    SocFieldCachedItemDelegate *m_table_delegate = new SocFieldCachedItemDelegate(this);
    m_table_edit_factory = new QItemEditorFactory();
    SocFieldCachedEditorCreator *m_table_edit_creator = new SocFieldCachedEditorCreator();
    // FIXME see QTBUG-30392
    m_table_edit_factory->registerEditor((QVariant::Type)qMetaTypeId< SocFieldCachedValue >(),
        m_table_edit_creator);
    m_table_delegate->setItemEditorFactory(m_table_edit_factory);
    m_value_table->setItemDelegate(m_table_delegate);

    m_sexy_display = new RegSexyDisplay(reg_ref, this);
    m_sexy_display->setFont(m_reg_font);

    m_desc = new QLabel(this);
    m_desc->setTextFormat(Qt::RichText);
    m_desc->setText(QString::fromStdString(m_reg.GetReg().desc));

    right_layout->addWidget(m_desc);
    right_layout->addLayout(top_layout);
    if(raw_val_layout)
        right_layout->addLayout(raw_val_layout);
    right_layout->addWidget(m_sexy_display);
    right_layout->addWidget(m_value_table);

    setTitle("Register Description");
    m_viewport = new QWidget;
    m_viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_viewport->setLayout(right_layout);
    m_scroll = new QScrollArea;
    m_scroll->setWidget(m_viewport);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_scroll, 1);
    setLayout(layout);
    AllowWrite(false);

    m_ignore_cell_change = false;
    // load data
    Reload();

    connect(m_raw_val_edit->GetLineEdit(), SIGNAL(returnPressed()), this, SLOT(OnRawRegValueReturnPressed()));
    connect(m_value_table, SIGNAL(cellChanged(int, int)), this, SLOT(OnRegFieldValueChanged(int, int)));
}

RegDisplayPanel::~RegDisplayPanel()
{
    delete m_table_edit_factory;
}

void RegDisplayPanel::Reload()
{
    const soc_dev_addr_t& dev_addr = m_reg.GetDevAddr();
    const soc_reg_t& reg = m_reg.GetReg();
    const soc_reg_addr_t& reg_addr = m_reg.GetRegAddr();
    soc_word_t value;
    BackendHelper helper(m_io_backend, m_reg);
    bool has_value = helper.ReadRegister(dev_addr.name.c_str(), reg_addr.name.c_str(), value);

    if(has_value)
    {
        m_raw_val_name->show();
        m_raw_val_edit->show();
        m_raw_val_edit->GetLineEdit()->setText(QString().sprintf("0x%08x", value));
    }
    else
    {
        m_raw_val_name->hide();
        m_raw_val_edit->hide();
    }

    m_ignore_cell_change = true;
    for(size_t row = 0; row < reg.field.size(); row++)
    {
        const soc_reg_field_t& field = reg.field[row];
        QTableWidgetItem *item = m_value_table->item(row, FieldValueColumn);
        QTableWidgetItem *desc_item = m_value_table->item(row, FieldMeaningColumn);
        if(has_value)
        {
            soc_word_t v = (value & field.bitmask()) >> field.first_bit;
            QString value_name;
            foreach(const soc_reg_field_value_t& rval, field.value)
                if(v == rval.value)
                    value_name = rval.name.c_str();
            item->setData(Qt::DisplayRole, QVariant::fromValue(SocFieldCachedValue(field, v)));
            if(value_name.size() != 0)
                desc_item->setText(value_name);
        }
        else
            item->setData(Qt::DisplayRole, QVariant::fromValue(SocFieldCachedValue()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if(m_allow_write)
            item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_ignore_cell_change = false;

    m_value_table->resizeColumnsToContents();
    m_value_table->horizontalHeader()->setStretchLastSection(true);
}

void RegDisplayPanel::AllowWrite(bool en)
{
    m_allow_write = en;
    if(m_raw_val_edit)
        m_raw_val_edit->SetReadOnly(m_io_backend->IsReadOnly() || !m_allow_write);
    Reload();
}

IoBackend::WriteMode RegDisplayPanel::EditModeToWriteMode(RegLineEdit::EditMode mode)
{
    switch(mode)
    {
        case RegLineEdit::Write: return IoBackend::Write;
        case RegLineEdit::Set: return IoBackend::Set;
        case RegLineEdit::Clear: return IoBackend::Clear;
        case RegLineEdit::Toggle: return IoBackend::Toggle;
        default: return IoBackend::Write;
    }
}

void RegDisplayPanel::OnRawRegValueReturnPressed()
{
    soc_word_t val;
    QLineEdit *edit = m_raw_val_edit->GetLineEdit();
    const SocFieldValidator *validator = dynamic_cast< const SocFieldValidator *>(edit->validator());
    QValidator::State state = validator->parse(edit->text(), val);
    if(state != QValidator::Acceptable)
        return;
    IoBackend::WriteMode mode = EditModeToWriteMode(m_raw_val_edit->GetMode());
    BackendHelper helper(m_io_backend, m_reg);
    helper.WriteRegister(m_reg.GetDevAddr().name.c_str(), m_reg.GetRegAddr().name.c_str(),
        val, mode);
    Reload();
}

void RegDisplayPanel::OnRegFieldValueChanged(int row, int col)
{
    if(m_ignore_cell_change || col != FieldValueColumn)
        return;
    QTableWidgetItem *item = m_value_table->item(row, col);
    SocFieldCachedValue val = item->data(Qt::DisplayRole).value< SocFieldCachedValue >();
    BackendHelper helper(m_io_backend, m_reg);
    soc_word_t regval;
    if(!helper.ReadRegister(m_reg.GetDevAddr().name.c_str(), m_reg.GetRegAddr().name.c_str(), regval))
        return;
    regval = (regval & ~val.field().bitmask()) |
        ((val.value() << val.field().first_bit) & val.field().bitmask());
    helper.WriteRegister(m_reg.GetDevAddr().name.c_str(), m_reg.GetRegAddr().name.c_str(),
        regval, IoBackend::Write);
    Reload();
}

QWidget *RegDisplayPanel::GetWidget()
{
    return this;
}

