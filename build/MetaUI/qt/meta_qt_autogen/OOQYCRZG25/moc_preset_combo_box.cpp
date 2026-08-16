/****************************************************************************
** Meta object code from reading C++ file 'preset_combo_box.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../MetaUI/qt/include/meta_qt/widgets/preset_combo_box.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'preset_combo_box.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_meta__qt__PresetComboBox_t {
    uint offsetsAndSizes[26];
    char stringdata0[25];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[5];
    char stringdata5[15];
    char stringdata6[9];
    char stringdata7[13];
    char stringdata8[15];
    char stringdata9[26];
    char stringdata10[4];
    char stringdata11[19];
    char stringdata12[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_meta__qt__PresetComboBox_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_meta__qt__PresetComboBox_t qt_meta_stringdata_meta__qt__PresetComboBox = {
    {
        QT_MOC_LITERAL(0, 24),  // "meta::qt::PresetComboBox"
        QT_MOC_LITERAL(25, 15),  // "preset_selected"
        QT_MOC_LITERAL(41, 0),  // ""
        QT_MOC_LITERAL(42, 11),  // "std::string"
        QT_MOC_LITERAL(54, 4),  // "name"
        QT_MOC_LITERAL(59, 14),  // "nlohmann::json"
        QT_MOC_LITERAL(74, 8),  // "snapshot"
        QT_MOC_LITERAL(83, 12),  // "preset_saved"
        QT_MOC_LITERAL(96, 14),  // "preset_deleted"
        QT_MOC_LITERAL(111, 25),  // "on_context_menu_requested"
        QT_MOC_LITERAL(137, 3),  // "pos"
        QT_MOC_LITERAL(141, 18),  // "on_index_activated"
        QT_MOC_LITERAL(160, 5)   // "index"
    },
    "meta::qt::PresetComboBox",
    "preset_selected",
    "",
    "std::string",
    "name",
    "nlohmann::json",
    "snapshot",
    "preset_saved",
    "preset_deleted",
    "on_context_menu_requested",
    "pos",
    "on_index_activated",
    "index"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_meta__qt__PresetComboBox[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   44,    2, 0x06,    1 /* Public */,
       7,    1,   49,    2, 0x06,    4 /* Public */,
       8,    1,   52,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    1,   55,    2, 0x08,    8 /* Private */,
      11,    1,   58,    2, 0x08,   10 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,   10,
    QMetaType::Void, QMetaType::Int,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject meta::qt::PresetComboBox::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_meta__qt__PresetComboBox.offsetsAndSizes,
    qt_meta_data_meta__qt__PresetComboBox,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_meta__qt__PresetComboBox_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PresetComboBox, std::true_type>,
        // method 'preset_selected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::string, std::false_type>,
        QtPrivate::TypeAndForceComplete<nlohmann::json, std::false_type>,
        // method 'preset_saved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::string, std::false_type>,
        // method 'preset_deleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::string, std::false_type>,
        // method 'on_context_menu_requested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'on_index_activated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void meta::qt::PresetComboBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PresetComboBox *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->preset_selected((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<nlohmann::json>>(_a[2]))); break;
        case 1: _t->preset_saved((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1]))); break;
        case 2: _t->preset_deleted((*reinterpret_cast< std::add_pointer_t<std::string>>(_a[1]))); break;
        case 3: _t->on_context_menu_requested((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 4: _t->on_index_activated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PresetComboBox::*)(std::string , nlohmann::json );
            if (_t _q_method = &PresetComboBox::preset_selected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PresetComboBox::*)(std::string );
            if (_t _q_method = &PresetComboBox::preset_saved; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PresetComboBox::*)(std::string );
            if (_t _q_method = &PresetComboBox::preset_deleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *meta::qt::PresetComboBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *meta::qt::PresetComboBox::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_meta__qt__PresetComboBox.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int meta::qt::PresetComboBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void meta::qt::PresetComboBox::preset_selected(std::string _t1, nlohmann::json _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void meta::qt::PresetComboBox::preset_saved(std::string _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void meta::qt::PresetComboBox::preset_deleted(std::string _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
