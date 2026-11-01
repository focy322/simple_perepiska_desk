#ifndef ATTACHMENTHELPER_H
#define ATTACHMENTHELPER_H

#include <QString>

struct Attachment;

class AttachmentHelper
{
public:
    static QString localFilePath(const Attachment &attachment);
    static QString category(const Attachment &attachment);
};

#endif // ATTACHMENTHELPER_H
