/************************************************************************
**
**  Copyright (C) 2015  Kevin B. Hendricks, Stratford ON
**
**  This file is part of Sigil.
**
**  Sigil is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Sigil is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Sigil.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#pragma once
#ifndef OPFRESOURCE_H
#define OPFRESOURCE_H

#include <memory>
#include "BookManipulation/GuideSemantics.h"
#include "ResourceObjects/XMLResource.h"
#include "ResourceObjects/OPFParser.h"

class HTMLResource;
class ImageResource;
class NCXResource;


class OPFResource : public XMLResource
{
    Q_OBJECT

public:

    /**
     * Constructor.
     *
     * @param fullfilepath The full path to the file that this
     *                     resource is representing.
     * @param parent The object's parent.
     */
    OPFResource(const QString &mainfolder, const QString &fullfilepath, QObject *parent = NULL);

    // inherited

    virtual bool RenameTo(const QString &new_filename);

    virtual ResourceType Type() const;

    virtual QString GetText() const;

    virtual void SetText(const QString &text);

    GuideSemantics::GuideSemanticType GetGuideSemanticTypeForResource(const Resource *resource) const;
    QString GetGuideSemanticNameForResource(Resource *resource);

    QHash <QString, QString> GetGuideSemanticNameForPaths();

    int GetReadingOrder(const HTMLResource *html_resource) const;
    QHash <Resource *, int> GetReadingOrderAll( const QList <Resource *> resources);

    QString GetMainIdentifierValue() const;

    void SaveToDisk(bool book_wide_save = false);

    QString GetPackageVersion() const;

    // Also creates such an ident if none was found
    QString GetUUIDIdentifierValue();

    void EnsureUUIDIdentifierPresent();

    QString AddNCXItem(const QString &ncx_path);

    void UpdateNCXOnSpine(const QString &new_ncx_id);

    void UpdateNCXLocationInManifest(const NCXResource *ncx);

    void AddModificationDateMeta();

    void AddSigilVersionMeta();

    bool IsCoverImage(const ImageResource *image_resource) const;

    void AutoFixWellFormedErrors();

    QStringList GetSpineOrderFilenames() const;

    /**
     * SetSpineOrderFromFilenames
     * The setter to complement GetSpineOrderFilenames()
     *
     * @param An list of the content file names that must be written
     *        in order to the spine.
     **/
    // void SetSpineOrderFromFilenames(const QStringList spineOrder);

    /**
     * Returns the book's Dublin Core metadata.
     *
     * @return The DC metadata, in the same format as the SetDCMetadata metadata parameter.
     */
    QList<MetaEntry> GetDCMetadata() const;

    /**
     * Returns the values for a specific dc: metadata name.
     *
     * @return A list of values
     */
    QList<QVariant> GetDCMetadataValues(QString text) const;

    QString GetRelativePathToRoot() const;

    void SetNavResource(HTMLResource* nav);
    HTMLResource* GetNavResource() const;

public slots:

    /**
     * Writes metadata to the OPF <metadata> element.
     *
     * @param metadata A list with meta information about the book.
     */

    void SetDCMetadata(const QList<MetaEntry>  &metadata);

    void AddResource(const Resource *resource);

    void AddCoverMetaForImage(const Resource *resource, OPFParser &p);

    void RemoveResource(const Resource *resource);

    void AddGuideSemanticType(HTMLResource *html_resource, GuideSemantics::GuideSemanticType new_type);

    void SetResourceAsCoverImage(ImageResource *image_resource);

    void UpdateSpineOrder(const QList<HTMLResource *> html_files);

    void ResourceRenamed(const Resource *resource, QString old_full_path);

    void UpdateManifestProperties(const QList<Resource *> resources);

private:

    /**
     * Determines if a cover image exists.
     *
     * @return \c true if a cover image exists.
     */
    bool CoverImageExists() const;

    bool IsCoverImageCheck(QString resource_id, const OPFParser& p) const;

    void RemoveCoverMetaForImage(const Resource *resource, OPFParser &p);

    // static void AppendToSpine(const QString &id);

    // static void RemoveFromSpine(const QString &id);

    // static void UpdateItemrefID(const QString &old_id, const QString &new_id);

    int GetMainIdentifier(const OPFParser &p) const;

    // CAN BE -1 which means no reference for resource
    int GetGuideReferenceForResourcePos(const Resource *resource, const OPFParser &p) const;

    void RemoveGuideReferenceForResource(const Resource *resource, OPFParser &p);

    GuideSemantics::GuideSemanticType GetGuideSemanticTypeForResource(const Resource *resource, const OPFParser &p) const;

    void SetGuideSemanticTypeForResource(GuideSemantics::GuideSemanticType type, const Resource *resource, OPFParser &p);

    void RemoveDuplicateGuideTypes(GuideSemantics::GuideSemanticType new_type, OPFParser &p);

        // CAN BE -1 means no cover meta element
    int GetCoverMeta(const OPFParser &p) const;

    QString GetResourceManifestID(const Resource *resource, const OPFParser &p) const;

    QHash<Resource *, QString> GetResourceManifestIDMapping(const QList<Resource *> resources, const OPFParser &p);

    void RemoveDCElements(OPFParser &p);

    /**
     * Writes simple metadata.
     *
     * @param metaname The name of the metadata to be written.
     * @param metavalue The value of the metadata to be written.
     * @param document The OPF DOM document.
     */
    void WriteSimpleMetadata(const QString &metaname, const QString &metavalue, OPFParser &p);

    /**
     * Writes the <identifier> elements.
     * The metaname will be used for the scheme.
     *
     * @param metaname The name of the metadata to be written.
     * @param metavalue The value of the metadata to be written.
     * @param document The OPF DOM document.
     */
    void WriteIdentifier(const QString &metaname, const QString &metavalue, OPFParser &p);

    QStringList GetRelativePathsToAllFilesInOEPBS() const;

    static QString GetOPFDefaultText(const QString &version);

    void FillWithDefaultText();

    QString GetUniqueID(const QString &preferred_id, const OPFParser &p) const;

    QString GetResourceMimetype(const Resource *resource) const;

    QString GetFileMimetype(const QString &filepath) const;

    void UpdateText(const OPFParser &p);

    QString ValidatePackageVersion(const QString &source);

    /**
     * Initializes m_Mimetypes.
     */
    void CreateMimetypes();

    ///////////////////////////////
    // PRIVATE MEMBER VARIABLES
    ///////////////////////////////

    /**
     * A mapping between file extensions
     * and appropriate MIME types.
     */
    QHash<QString, QString> m_Mimetypes;

    HTMLResource * m_NavResource;
    bool m_WarnedAboutVersion;
};

#endif // OPFRESOURCE_H
