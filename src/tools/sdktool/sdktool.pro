include(../../qtcreatortool.pri)

QT -= concurrent gui widgets testlib

QT += xmlpatterns

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../shared/qtcreator_pch.h

UTILS = $$PWD/../../libs/utils
DEFINES += UTILS_LIBRARY
win32: LIBS += -luser32 -lshell32

DEFINES += QT_NO_URL_CAST_FROM_STRING

SOURCES += \
    main.cpp \
    addabiflavor.cpp \
    addcmakeoperation.cpp \
    adddebuggeroperation.cpp \
    adddeviceoperation.cpp \
    addkeysoperation.cpp \
    addkitoperation.cpp \
    addqtoperation.cpp \
    addsfdkbuildengineoperation.cpp \
    addsfdkdevicemodeloperation.cpp \
    addsfdkemulatoroperation.cpp \
    addtoolchainoperation.cpp \
    addvalueoperation.cpp \
    findkeyoperation.cpp \
    findvalueoperation.cpp \
    getoperation.cpp \
    operation.cpp \
    rmcmakeoperation.cpp \
    rmdebuggeroperation.cpp \
    rmdeviceoperation.cpp \
    rmkeysoperation.cpp \
    rmkitoperation.cpp \
    rmqtoperation.cpp \
    rmsfdkbuildengineoperation.cpp \
    rmsfdkdevicemodeloperation.cpp \
    rmsfdkemulatoroperation.cpp \
    rmtoolchainoperation.cpp \
    settings.cpp \
    sfdkutils.cpp \
    $$UTILS/environment.cpp \
    $$UTILS/fileutils.cpp \
    $$UTILS/hostosinfo.cpp \
    $$UTILS/namevaluedictionary.cpp \
    $$UTILS/namevalueitem.cpp \
    $$UTILS/persistentsettings.cpp \
    $$UTILS/qtcassert.cpp \
    $$UTILS/qtcprocess.cpp \
    $$UTILS/savefile.cpp \
    $$UTILS/stringutils.cpp

HEADERS += \
    addabiflavor.h \
    addcmakeoperation.h \
    adddebuggeroperation.h \
    adddeviceoperation.h \
    addkeysoperation.h \
    addkitoperation.h \
    addqtoperation.h \
    addsfdkbuildengineoperation.h \
    addsfdkdevicemodeloperation.h \
    addsfdkemulatoroperation.h \
    addtoolchainoperation.h \
    addvalueoperation.h \
    findkeyoperation.h \
    findvalueoperation.h \
    getoperation.h \
    operation.h \
    rmcmakeoperation.h \
    rmdebuggeroperation.h \
    rmdeviceoperation.h \
    rmkeysoperation.h \
    rmkitoperation.h \
    rmqtoperation.h \
    rmsfdkbuildengineoperation.h \
    rmsfdkdevicemodeloperation.h \
    rmsfdkemulatoroperation.h \
    rmtoolchainoperation.h \
    settings.h \
    sfdkutils.h \
    $$UTILS/environment.h \
    $$UTILS/fileutils.h \
    $$UTILS/hostosinfo.h \
    $$UTILS/namevaluedictionary.h \
    $$UTILS/namevalueitem.h \
    $$UTILS/persistentsettings.h \
    $$UTILS/qtcassert.h \
    $$UTILS/qtcprocess.h \
    $$UTILS/savefile.h \
    $$UTILS/porting.h

macos {
    OBJECTIVE_SOURCES += \
        $$UTILS/fileutils_mac.mm \

    HEADERS += \
        $$UTILS/fileutils_mac.h \

    LIBS += -framework Foundation
}

# Generate app_version.h also here, so building sdktool does not require
# running qmake on src/app/
appversion.input = $$PWD/../../app/app_version.h.in
appversion.output = $$OUT_PWD/app/app_version.h
QMAKE_SUBSTITUTES += appversion
INCLUDEPATH += $$OUT_PWD

isEmpty(SDKTOOL_DATA_PATH) {
    macos:DEFINES += $$shell_quote(DATA_PATH=\"..\")
    else:win32:DEFINES += $$shell_quote(DATA_PATH=\"../share/qtcreator\")
    else:DEFINES += $$shell_quote(DATA_PATH=\"../../share/qtcreator\")
} else {
    DEFINES += $$shell_quote(DATA_PATH=\"$$SDKTOOL_DATA_PATH\")
}
