include(../qtcreator.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

qtc_human_user_tool {
    DESTDIR = $$IDE_BIN_PATH
    target.path = $$INSTALL_BIN_PATH
} else {
    DESTDIR = $$IDE_LIBEXEC_PATH
    target.path  = $$INSTALL_LIBEXEC_PATH
}

RPATH_BASE = $$DESTDIR
include(rpath.pri)

INSTALLS += target
