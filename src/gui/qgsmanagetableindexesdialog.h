/***************************************************************************
    qgsmanagetableindexesdialog.h
                             -------------------
    begin                : October 2026
    copyright            : (C) 2026 Dominik Cindric
    email                : dominik.cindric@lutraconsulting.co.uk
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMANAGETABLEINDEXESDIALOG_H
#define QGSMANAGETABLEINDEXESDIALOG_H

#include "qgis_gui.h"
#include "qgis_sip.h"

#include <QDialog>
#include <QStringList>

class QStandardItemModel;
class QTableView;
class QPushButton;
class QgsAbstractDatabaseProviderConnection;

/**
 * \ingroup gui
 * \brief A dialog to list, create and delete (non-spatial) indexes on a database table.
 * \since QGIS 4.6
 */
class GUI_EXPORT QgsManageTableIndexesDialog : public QDialog
{
    Q_OBJECT

  public:
    /**
     * Constructor.
     * \param connection database connection (not owned); must outlive the dialog.
     * \param schema schema name (empty if the backend has no schema concept).
     * \param table table name.
     * \param fields names of columns on \a table, used to populate the Add Index dialog's column picker.
     * \param parent parent widget.
     */
    QgsManageTableIndexesDialog( QgsAbstractDatabaseProviderConnection *connection, const QString &schema, const QString &table, const QStringList &fields, QWidget *parent SIP_TRANSFERTHIS = nullptr );

  private slots:
    void refresh();
    void addIndex();
    void deleteSelectedIndex();

  private:
    QgsAbstractDatabaseProviderConnection *mConnection = nullptr;
    QString mSchema;
    QString mTable;
    QStringList mFields;

    QTableView *mView = nullptr;
    QStandardItemModel *mModel = nullptr;
    QPushButton *mDeleteButton = nullptr;
};

#endif // QGSMANAGETABLEINDEXESDIALOG_H
