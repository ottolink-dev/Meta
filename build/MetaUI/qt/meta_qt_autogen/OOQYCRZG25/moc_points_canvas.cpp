/****************************************************************************
** Meta object code from reading C++ file 'points_canvas.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../MetaUI/qt/include/meta_qt/widgets/points_canvas.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'points_canvas.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_meta__qt__PointsCanvas_t {
    uint offsetsAndSizes[8];
    char stringdata0[23];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_meta__qt__PointsCanvas_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_meta__qt__PointsCanvas_t qt_meta_stringdata_meta__qt__PointsCanvas = {
    {
        QT_MOC_LITERAL(0, 22),  // "meta::qt::PointsCanvas"
        QT_MOC_LITERAL(23, 14),  // "points_changed"
        QT_MOC_LITERAL(38, 0),  // ""
        QT_MOC_LITERAL(39, 10)   // "drag_ended"
    },
    "meta::qt::PointsCanvas",
    "points_changed",
    "",
    "drag_ended"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_meta__qt__PointsCanvas[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   26,    2, 0x06,    1 /* Public */,
       3,    0,   27,    2, 0x06,    2 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject meta::qt::PointsCanvas::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_meta__qt__PointsCanvas.offsetsAndSizes,
    qt_meta_data_meta__qt__PointsCanvas,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_meta__qt__PointsCanvas_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PointsCanvas, std::true_type>,
        // method 'points_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'drag_ended'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void meta::qt::PointsCanvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PointsCanvas *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->points_changed(); break;
        case 1: _t->drag_ended(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PointsCanvas::*)();
            if (_t _q_method = &PointsCanvas::points_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PointsCanvas::*)();
            if (_t _q_method = &PointsCanvas::drag_ended; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject *meta::qt::PointsCanvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *meta::qt::PointsCanvas::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_meta__qt__PointsCanvas.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int meta::qt::PointsCanvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void meta::qt::PointsCanvas::points_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void meta::qt::PointsCanvas::drag_ended()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
