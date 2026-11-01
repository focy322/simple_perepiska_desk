#include "attachmenthelper.h"
#include "utils/file_extensions.h"
#include "utils/parsed_objects.h"
#include "utils/paths.h"

#include <QDir>
#include <QFileInfo>

QString AttachmentHelper::localFilePath(const Attachment &attachment)
{
    if (!attachment.localPath.isEmpty() && QFileInfo(attachment.localPath).isFile())
        return attachment.localPath;
    if (!attachment.fileName.isEmpty()) {
        const QString path = QDir(appDownloadsDir).filePath(attachment.fileName);
        if (QFileInfo(path).isFile())
            return path;
    }
    return {};
}

QString AttachmentHelper::category(const Attachment &attachment)
{
    const QString path = localFilePath(attachment);
    if (!path.isEmpty())
        return defineFileCategory(path);

    const QString detected = defineFileCategory(attachment.fileName, attachment.contentType);
    if (detected != "document")
        return detected;

    const QString hint = attachment.category.trimmed().toLower();
    if (hint == "image" || hint == "video" || hint == "audio")
        return hint;
    return QStringLiteral("document");
}
