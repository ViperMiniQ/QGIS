/***************************************************************************
    qgsmanagetableindexesdialog.cpp
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

#include "qgsmanagetableindexesdialog.h"

#include "qgsabstractdatabaseproviderconnection.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

QgsManageTableIndexesDialog::QgsManageTableIndexesDialog( QgsAbstractDatabaseProviderConnection *connection, const QString &schema, const QString &table, const QStringList &fields, QWidget *parent )
  : QDialog( parent )
  , mConnection( connection )
  , mSchema( schema )
  , mTable( table )
  , mFields( fields )
{
  setWindowTitle( tr( "Indexes on %1" ).arg( table ) );
  resize( 500, 320 );

  QVBoxLayout *mainLayout = new QVBoxLayout( this );

  mModel = new QStandardItemModel( 0, 2, this );
  mModel->setHorizontalHeaderLabels( { tr( "Name" ), tr( "Column(s)" ) } );

  mView = new QTableView( this );
  mView->setModel( mModel );
  mView->setSelectionBehavior( QAbstractItemView::SelectRows );
  mView->setSelectionMode( QAbstractItemView::SingleSelection );
  mView->setEditTriggers( QAbstractItemView::NoEditTriggers );
  mView->horizontalHeader()->setStretchLastSection( true );
  mView->verticalHeader()->setVisible( false );
  mainLayout->addWidget( mView );

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  QPushButton *addButton = new QPushButton( tr( "Add Index…" ), this );
  mDeleteButton = new QPushButton( tr( "Delete Index" ), this );
  mDeleteButton->setEnabled( false );
  buttonLayout->addWidget( addButton );
  buttonLayout->addWidget( mDeleteButton );
  buttonLayout->addStretch( 1 );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close, this );
  buttonLayout->addWidget( bbox );
  mainLayout->addLayout( buttonLayout );

  connect( addButton, &QPushButton::clicked, this, &QgsManageTableIndexesDialog::addIndex );
  connect( mDeleteButton, &QPushButton::clicked, this, &QgsManageTableIndexesDialog::deleteSelectedIndex );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::accept );
  connect( mView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() { mDeleteButton->setEnabled( !mView->selectionModel()->selectedRows().isEmpty() ); } );

  refresh();
}

void QgsManageTableIndexesDialog::refresh()
{
  mModel->removeRows( 0, mModel->rowCount() );

  if ( !mConnection )
    return;

  QMap<QString, QStringList> indexes;
  try
  {
    indexes = mConnection->tableIndexes( mSchema, mTable );
  }
  catch ( const QgsProviderConnectionException &ex )
  {
    QMessageBox::critical( this, tr( "List Indexes" ), tr( "Failed to list indexes on '%1': %2" ).arg( mTable, ex.what() ) );
    return;
  }

  for ( auto it = indexes.constBegin(); it != indexes.constEnd(); ++it )
  {
    QStandardItem *nameItem = new QStandardItem( it.key() );
    QStandardItem *colsItem = new QStandardItem( it.value().join( ", "_L1 ) );
    mModel->appendRow( { nameItem, colsItem } );
  }

  mView->resizeColumnToContents( 0 );
  mDeleteButton->setEnabled( false );
}

void QgsManageTableIndexesDialog::addIndex()
{
  if ( !mConnection || mFields.isEmpty() )
    return;

  QDialog addDlg( this );
  addDlg.setWindowTitle( tr( "Create Index" ) );

  QComboBox *colCombo = new QComboBox( &addDlg );
  colCombo->addItems( mFields );
  QLineEdit *nameEdit = new QLineEdit( &addDlg );
  QCheckBox *uniqueCheck = new QCheckBox( tr( "Unique" ), &addDlg );

  auto updateDefaultName = [this, colCombo, nameEdit]() { nameEdit->setText( u"idx_%1_%2"_s.arg( mTable, colCombo->currentText() ) ); };
  updateDefaultName();
  connect( colCombo, &QComboBox::currentTextChanged, &addDlg, updateDefaultName );

  QDialogButtonBox *bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &addDlg );
  connect( bb, &QDialogButtonBox::accepted, &addDlg, &QDialog::accept );
  connect( bb, &QDialogButtonBox::rejected, &addDlg, &QDialog::reject );

  QFormLayout *form = new QFormLayout( &addDlg );
  form->addRow( tr( "Column" ), colCombo );
  form->addRow( tr( "Index name" ), nameEdit );
  form->addRow( QString(), uniqueCheck );
  form->addRow( bb );

  if ( addDlg.exec() != QDialog::Accepted )
    return;

  const QString column = colCombo->currentText();
  const QString name = nameEdit->text().trimmed();
  if ( column.isEmpty() || name.isEmpty() )
    return;

  try
  {
    mConnection->createIndex( mSchema, mTable, column, name, uniqueCheck->isChecked() );
  }
  catch ( const QgsProviderConnectionException &ex )
  {
    QMessageBox::critical( this, tr( "Create Index" ), tr( "Failed to create index '%1': %2" ).arg( name, ex.what() ) );
    return;
  }

  refresh();
}

void QgsManageTableIndexesDialog::deleteSelectedIndex()
{
  const QModelIndexList selected = mView->selectionModel()->selectedRows();
  if ( selected.isEmpty() )
    return;

  const QString indexName = mModel->item( selected.first().row(), 0 )->text();

  if ( QMessageBox::question( this, tr( "Delete Index" ), tr( "Delete index '%1'?" ).arg( indexName ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
    return;

  try
  {
    mConnection->deleteIndex( mSchema, mTable, indexName );
  }
  catch ( const QgsProviderConnectionException &ex )
  {
    QMessageBox::critical( this, tr( "Delete Index" ), tr( "Failed to delete index '%1': %2" ).arg( indexName, ex.what() ) );
    return;
  }

  refresh();
}
