/****************************************************************************
** Meta object code from reading C++ file 'vector_canvas.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../MetaUI/qt/include/meta_qt/widgets/vector_canvas.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vector_canvas.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_meta__qt__VectorCanvas_t {
    uint offsetsAndSizes[20];
    char stringdata0[23];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[2];
    char stringdata5[11];
    char stringdata6[18];
    char stringdata7[4];
    char stringdata8[14];
    char stringdata9[4];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_meta__qt__VectorCanvas_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_meta__qt__VectorCanvas_t qt_meta_stringdata_meta__qt__VectorCanvas = {
    {
        QT_MOC_LITERAL(0, 22),  // "meta::qt::VectorCanvas"
        QT_MOC_LITERAL(23, 13),  // "value_changed"
        QT_MOC_LITERAL(37, 0),  // ""
        QT_MOC_LITERAL(38, 9),  // "glm::vec2"
        QT_MOC_LITERAL(48, 1),  // "v"
        QT_MOC_LITERAL(50, 10),  // "drag_ended"
        QT_MOC_LITERAL(61, 17),  // "magnitude_changed"
        QT_MOC_LITERAL(79, 3),  // "mag"
        QT_MOC_LITERAL(83, 13),  // "angle_changed"
        QT_MOC_LITERAL(97, 3)   // "deg"
    },
    "meta::qt::VectorCanvas",
    "value_changed",
    "",
    "glm::vec2",
    "v",
    "drag_ended",
    "magnitude_changed",
    "mag",
    "angle_changed",
    "deg"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_meta__qt__VectorCanvas[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   38,    2, 0x06,    1 /* Public */,
       5,    1,   41,    2, 0x06,    3 /* Public */,
       6,    1,   44,    2, 0x06,    5 /* Public */,
       8,    1,   47,    2, 0x06,    7 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Float,    7,
    QMetaType::Void, QMetaType::Float,    9,

       0        // eod
};

Q_CONSTINIT const QMetaObject meta::qt::VectorCanvas::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_meta__qt__VectorCanvas.offsetsAndSizes,
    qt_meta_data_meta__qt__VectorCanvas,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_meta__qt__VectorCanvas_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<VectorCanvas, std::true_type>,
        // method 'value_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<glm::vec2, std::false_type>,
        // method 'drag_ended'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<glm::vec2, std::false_type>,
        // method 'magnitude_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        // method 'angle_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>
    >,
    nullptr
} };

void meta::qt::VectorCanvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VectorCanvas *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->value_changed((*reinterpret_cast< std::add_pointer_t<glm::vec2>>(_a[1]))); break;
        case 1: _t->drag_ended((*reinterpret_cast< std::add_pointer_t<glm::vec2>>(_a[1]))); break;
        case 2: _t->magnitude_changed((*reinterpret_cast< std::add_pointer_t<float>>(_a[1]))); break;
        case 3: _t->angle_changed((*reinterpret_cast< std::add_pointer_t<float>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VectorCanvas::*)(glm::vec2 );
            if (_t _q_method = &VectorCanvas::value_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VectorCanvas::*)(glm::vec2 );
            if (_t _q_method = &VectorCanvas::drag_ended; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VectorCanvas::*)(float );
            if (_t _q_method = &VectorCanvas::magnitude_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (VectorCanvas::*)(float );
            if (_t _q_method = &VectorCanvas::angle_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *meta::qt::VectorCanvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *meta::qt::VectorCanvas::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_meta__qt__VectorCanvas.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int meta::qt::VectorCanvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void meta::qt::VectorCanvas::value_changed(glm::vec2 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void meta::qt::VectorCanvas::drag_ended(glm::vec2 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void meta::qt::VectorCanvas::magnitude_changed(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void meta::qt::VectorCanvas::angle_changed(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
