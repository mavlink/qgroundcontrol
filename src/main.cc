/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Main executable
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include "QGC.h"
#include <QtGlobal>
#include <QApplication>
#include <QIcon>
#include <QSslSocket>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QHostAddress>
#include <QUdpSocket>
#include <QtPlugin>
#include <QStringListModel>
#include "QGCApplication.h"
#include "AppMessages.h"

#ifndef __mobile__
    #include "QGCSerialPortInfo.h"
    #include "RunGuard.h"
#endif

#ifdef UNITTEST_BUILD
    #include "UnitTest.h"
#endif

#ifdef QT_DEBUG
    #include "CmdLineOptParser.h"
    #ifdef Q_OS_WIN
        #include <crtdbg.h>
    #endif
#endif

#ifdef QGC_ENABLE_BLUETOOTH
#include <QtBluetooth/QBluetoothSocket>
#endif

#include <iostream>
#include "QGCMapEngine.h"

/* SDL does ugly things to main() */
#ifdef main
#undef main
#endif

#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    Q_DECLARE_METATYPE(QGCSerialPortInfo)
#endif
#endif

#ifdef Q_OS_WIN

#include <windows.h>

/// @brief CRT Report Hook installed using _CrtSetReportHook. We install this hook when
/// we don't want asserts to pop a dialog on windows.
int WindowsCrtReportHook(int reportType, char* message, int* returnValue)
{
    Q_UNUSED(reportType);

    std::cerr << message << std::endl;  // Output message to stderr
    *returnValue = 0;                   // Don't break into debugger
    return true;                        // We handled this fully ourselves
}

#endif

#if defined(__android__)
#include <jni.h>
#include "JoystickAndroid.h"
#if defined(QGC_ENABLE_PAIRING)
#include "PairingManager.h"
#endif
#if !defined(NO_SERIAL_LINK)
#include "qserialport.h"
#endif

static jobject _class_loader = nullptr;
static jobject _context = nullptr;

//-----------------------------------------------------------------------------
extern "C" {
    void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader;
    }
}

//-----------------------------------------------------------------------------
static void
gst_android_init(JNIEnv* env, jobject context)
{
    jobject class_loader = nullptr;

    jclass context_cls = env->GetObjectClass(context);
    if (!context_cls) {
        return;
    }

    jmethodID get_class_loader_id = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    class_loader = env->CallObjectMethod(context, get_class_loader_id);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    _context = env->NewGlobalRef(context);
    _class_loader = env->NewGlobalRef (class_loader);
}

//-----------------------------------------------------------------------------
static const char kJniClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

void setNativeMethods(void)
{
    JNINativeMethod javaMethods[] {
        {"nativeInit", "()V", reinterpret_cast<void *>(gst_android_init)}
    };

    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
        qWarning() << "Couldn't find class:" << kJniClassName;
        return;
    }

    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qWarning() << "Error registering methods: " << val;
    } else {
        qDebug() << "Main Native Functions Registered";
    }

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
}

//-----------------------------------------------------------------------------
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    Q_UNUSED(reserved);

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    setNativeMethods();

#if defined(QGC_GST_STREAMING)
    // Tell the androidmedia plugin about the Java VM
    gst_amc_jni_set_java_vm(vm);
#endif

 #if !defined(NO_SERIAL_LINK)
    QSerialPort::setNativeMethods();
 #endif

    JoystickAndroid::setNativeMethods();

#if defined(QGC_ENABLE_PAIRING)
    PairingManager::setNativeMethods();
#endif

    return JNI_VERSION_1_6;
}
#endif

//-----------------------------------------------------------------------------
#ifdef __android__
#include <QtAndroid>
bool checkAndroidWritePermission() {
    QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
    if(r == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync( QStringList() << "android.permission.WRITE_EXTERNAL_STORAGE" );
        r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
        if(r == QtAndroid::PermissionResult::Denied) {
             return false;
        }
   }
   return true;
}
#endif

//-----------------------------------------------------------------------------
/**
 * @brief Starts the application
 *
 * @param argc Number of commandline arguments
 * @param argv Commandline arguments
 * @return exit code, 0 for normal exit and !=0 for error cases
 */

int main(int argc, char *argv[])
{
#ifndef __mobile__
    RunGuard guard("QGroundControlRunGuardKey");
    if (!guard.tryToRun()) {
        // QApplication is necessary to use QMessageBox
        QApplication errorApp(argc, argv);
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QObject::tr("A second instance of %1 is already running. Please close the other instance and try again.").arg(QGC_APPLICATION_NAME)
        );
        return -1;
    }
#endif

    //-- Record boot time
    QGC::initTimer();

#ifdef Q_OS_UNIX
    //Force writing to the console on UNIX/BSD devices
    if (!qEnvironmentVariableIsSet("QT_LOGGING_TO_CONSOLE"))
        qputenv("QT_LOGGING_TO_CONSOLE", "1");
#endif

    // install the message handler
    AppMessages::installHandler();

#ifdef Q_OS_MAC
#ifndef __ios__
    // Prevent Apple's app nap from screwing us over
    // tip: the domain can be cross-checked on the command line with <defaults domains>
    QProcess::execute("defaults write org.qgroundcontrol.qgroundcontrol NSAppSleepDisabled -bool YES");
#endif
#endif

#ifdef Q_OS_WIN
    // Set our own OpenGL buglist
    qputenv("QT_OPENGL_BUGLIST", ":/opengl/resources/opengl/buglist.json");

    // Allow for command line override of renderer
    for (int i = 0; i < argc; i++) {
        const QString arg(argv[i]);
        if (arg == QStringLiteral("-angle")) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            break;
        } else if (arg == QStringLiteral("-swrast")) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
            break;
        }
    }
#endif

    // The following calls to qRegisterMetaType are done to silence debug output which warns
    // that we use these types in signals, and without calling qRegisterMetaType we can't queue
    // these signals. In general we don't queue these signals, but we do what the warning says
    // anyway to silence the debug output.
#ifndef NO_SERIAL_LINK
    qRegisterMetaType<QSerialPort::SerialPortError>();
#endif
#ifdef QGC_ENABLE_BLUETOOTH
    qRegisterMetaType<QBluetoothSocket::SocketError>();
    qRegisterMetaType<QBluetoothServiceInfo>();
#endif
    qRegisterMetaType<QAbstractSocket::SocketError>();
#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    qRegisterMetaType<QGCSerialPortInfo>();
#endif
#endif

    // We statically link our own QtLocation plugin

#ifdef Q_OS_WIN
    // In Windows, the compiler doesn't see the use of the class created by Q_IMPORT_PLUGIN
#pragma warning( disable : 4930 4101 )
#endif

    Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryQGC)

    bool runUnitTests = false;          // Run unit tests

#ifdef QT_DEBUG
    // We parse a small set of command line options here prior to QGCApplication in order to handle the ones
    // which need to be handled before a QApplication object is started.

    bool stressUnitTests = false;       // Stress test unit tests
    bool quietWindowsAsserts = false;   // Don't let asserts pop dialog boxes

    QString unitTestOptions;
    CmdLineOpt_t rgCmdLineOptions[] = {
        { "--unittest",             &runUnitTests,          &unitTestOptions },
        { "--unittest-stress",      &stressUnitTests,       &unitTestOptions },
        { "--no-windows-assert-ui", &quietWindowsAsserts,   nullptr },
        // Add additional command line option flags here
    };

    ParseCmdLineOptions(argc, argv, rgCmdLineOptions, sizeof(rgCmdLineOptions)/sizeof(rgCmdLineOptions[0]), false);
    if (stressUnitTests) {
        runUnitTests = true;
    }

    if (quietWindowsAsserts) {
#ifdef Q_OS_WIN
        _CrtSetReportHook(WindowsCrtReportHook);
#endif
    }

#ifdef Q_OS_WIN
    if (runUnitTests) {
        // Don't pop up Windows Error Reporting dialog when app crashes. This prevents TeamCity from
        // hanging.
        DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }
#endif
#endif // QT_DEBUG

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGCApplication* app = new QGCApplication(argc, argv, runUnitTests);
    Q_CHECK_PTR(app);
    if(app->isErrorState()) {
        app->exec();
        return -1;
    }

#ifdef Q_OS_LINUX
    QApplication::setWindowIcon(QIcon(":/res/resources/icons/qgroundcontrol.ico"));
#endif /* Q_OS_LINUX */

    // There appears to be a threading issue in qRegisterMetaType which can cause it to throw a qWarning
    // about duplicate type converters. This is caused by a race condition in the Qt code. Still working
    // with them on tracking down the bug. For now we register the type which is giving us problems here
    // while we only have the main thread. That should prevent it from hitting the race condition later
    // on in the code.
    qRegisterMetaType<QList<QPair<QByteArray,QByteArray> > >();

    app->_initCommon();
    //-- Initialize Cache System
    getQGCMapEngine()->init();

    int exitCode = 0;

#ifdef UNITTEST_BUILD
    if (runUnitTests) {
        for (int i=0; i < (stressUnitTests ? 20 : 1); i++) {
            if (!app->_initForUnitTests()) {
                return -1;
            }

            // Run the test
            int failures = UnitTest::run(unitTestOptions);
            if (failures == 0) {
                qDebug() << "ALL TESTS PASSED";
                exitCode = 0;
            } else {
                qDebug() << failures << " TESTS FAILED!";
                exitCode = -failures;
                break;
            }
        }
    } else
#endif
    {

#ifdef __android__
        checkAndroidWritePermission();
#endif
        if (!app->_initForNormalAppBoot()) {
            return -1;
        }
        exitCode = app->exec();
    }

    app->_shutdown();
    delete app;
    //-- Shutdown Cache System
    destroyMapEngine();

    qDebug() << "After app delete";

    return exitCode;
}
