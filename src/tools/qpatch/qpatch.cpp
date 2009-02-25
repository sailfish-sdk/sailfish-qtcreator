
#include <QtCore>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    if (args.size() != 3) {
        std::cerr << "Usage: qpatch file.list oldQtDir newQtDir" << std::endl;
        return EXIT_FAILURE;
    }

    const QString files = args.takeFirst();
    const QByteArray qtDirPath = QFile::encodeName(args.takeFirst());
    const QByteArray newQtPath = QFile::encodeName(args.takeFirst());

    if (qtDirPath.size() < newQtPath.size()) {
        std::cerr << "qpatch: error: newQtDir needs to be less than " << qtDirPath.size() << " characters."
                << std::endl;
        return EXIT_FAILURE;
    }

    QFile fn(files);
    if (! fn.open(QFile::ReadOnly)) {
        std::cerr << "qpatch: error: file not found" << std::endl;
        return EXIT_FAILURE;
    }

    QStringList filesToPatch, textFilesToPatch;
    bool readingTextFilesToPatch = false;

    // read the input file
    QTextStream in(&fn);

    forever {
        const QString line = in.readLine();

        if (line.isNull())
            break;

        else if (line.isEmpty())
            continue;

        else if (line.startsWith(QLatin1String("%%")))
            readingTextFilesToPatch = true;

        else if (readingTextFilesToPatch)
            textFilesToPatch.append(line);

        else
            filesToPatch.append(line);
    }

    foreach (QString fileName, filesToPatch) {
        QString prefix = newQtPath;

        if (! prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');

        fileName.prepend(prefix);

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "qpatch: warning: file `" << qPrintable(fileName) << "' not found" << std::endl;
            continue;
        }

        const QByteArray source = file.readAll();
        file.close();

        int index = 0;

        if (! file.open(QFile::WriteOnly | QFile::Truncate)) {
            std::cerr << "qpatch: error: file `" << qPrintable(fileName) << "' not writable" << std::endl;
            continue;
        }

        forever {
            int start = source.indexOf(qtDirPath, index);
            if (start == -1)
                break;

            int endOfString = start;
            for (; endOfString < source.size(); ++endOfString) {
                if (! source.at(endOfString))
                    break;
            }

            ++endOfString; // include the '\0'

            if (index != start)
                file.write(source.constData() + index, start - index);

            int length = endOfString - start;
            QVector<char> s;

            for (const char *x = newQtPath.constData(); x != newQtPath.constEnd(); ++x)
                s.append(*x);

            const int qtDirPathLength = qtDirPath.size();

            for (const char *x = source.constData() + start + qtDirPathLength;
                    x != source.constData() + endOfString; ++x)
                s.append(*x);

            const int oldSize = s.size();

            for (int i = oldSize; i < length; ++i)
                s.append('\0');

#if 0
            std::cout << "replace string: " << source.mid(start, endOfString - start).constData()
                    << " with: " << s.constData() << std::endl;
#endif

            file.write(s.constData(), s.size());

            index = endOfString;
        }

        if (index != source.size())
            file.write(source.constData() + index, source.size() - index);
    }

    foreach (QString fileName, textFilesToPatch) {
        QString prefix = newQtPath;

        if (! prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');

        fileName.prepend(prefix);

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "qpatch: warning: file `" << qPrintable(fileName) << "' not found" << std::endl;
            continue;
        }

        QByteArray source = file.readAll();
        file.close();

        if (! file.open(QFile::WriteOnly | QFile::Truncate)) {
            std::cerr << "qpatch: error: file `" << qPrintable(fileName) << "' not writable" << std::endl;
            continue;
        }

        source.replace(qtDirPath, newQtPath);
        file.write(source);
    }

    return 0;
}
