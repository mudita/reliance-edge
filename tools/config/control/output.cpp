/*             ----> DO NOT REMOVE THE FOLLOWING NOTICE <----

                   Copyright (c) 2014-2018 Datalight, Inc.
                       All Rights Reserved Worldwide.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; use version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but "AS-IS," WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/*  Businesses and individuals that for commercial or other reasons cannot
    comply with the terms of the GPLv2 license may obtain a commercial license
    before incorporating Reliance Edge into proprietary software for
    distribution in any form.  Visit http://www.datalight.com/reliance-edge for
    more information.
*/
#include "ui/errordialog.h"
#include "debug.h"
#include "output.h"
#include "allsettings.h"
#include "volumesettings.h"
#include "version.h"

Output::Output(QWidget *parentWin)
    : parentWindow(parentWin),
      fileDialog(NULL),
      errorDialog(new ErrorDialog(parentWindow)),
      isSaving(false)
{
    connect(errorDialog, SIGNAL(results(ErrorDialog::Result)),
            this, SLOT(errorDialog_results(ErrorDialog::Result)));
}

void Output::TrySave(const QString & headerPath, const QString & codefilePath)
{
    Q_ASSERT(errorDialog != NULL);

    if(isSaving)
    {
        // Shouldn't be possible because the UI is blocked while
        // isSaving is true.
        Q_ASSERT(false);
        emit results(OutResultErrorBusy, QString::null, QString::null);
    }

    currHeaderPath = headerPath;
    currCodefilePath = codefilePath;

    if(errorDialog->isVisible())
    {
        errorDialog->close();
    }

    isSaving = true;

    QStringList errors;
    QStringList warnings;
    AllSettings::GetErrors(errors, warnings);

    if(errors.count() > 0 || warnings.count() > 0)
    {
        if(errors.count() > 0)
        {
            // No more chance of saving at this point.
            isSaving = false;
            errorDialog->SetErrorText("Please correct the following invalid values before continuing:");
            errorDialog->ShowErrorsInfo(errors, warnings);
        }
        else
        {
            errorDialog->SetErrorText("Continue despite the following warnings?");
            errorDialog->ShowErrorsAction(errors, warnings);

            // Resumes in errorDialog_results if the user chooses
            // to continue
        }
    }
    else
    {
        // No errors were found; go straight to output.
        doOutput();
        isSaving = false;
    }
}

void Output::ShowErrors(bool showIfNoErrors)
{
    if(isSaving)
    {
        Q_ASSERT(false); //Don't want this happening.
        emit results(OutResultErrorBusy, QString::null, QString::null);
    }
    Q_ASSERT(errorDialog != NULL);

    QStringList errors, warnings;
    AllSettings::GetErrors(errors, warnings);

    if(errors.count() > 0 || warnings.count() > 0)
    {
        errorDialog->SetErrorText("The following errors and warnings were found:");
        errorDialog->ShowErrorsInfo(errors, warnings);
    }
    else if(showIfNoErrors)
    {
        errorDialog->SetErrorText("No errors or warnings found.");
        errorDialog->ShowErrorsInfo(errors, warnings);
    }
    // else do nothing and return.
}

void Output::doOutput()
{
    QFile fileHeader, fileCodefile;
    QString headerPath, codefilePath;
    bool success;
    const QString headMessage = QString(
"/*  THIS FILE WAS GENERATED BY THE DATALIGHT RELIANCE EDGE CONFIGURATION\n"
"    UTILITY.  DO NOT MODIFY.\n"
"\n"
"    Generated by configuration utility version " + QString(CONFIG_VERSION) + "\n"
"*/\n");

    // If we were given file paths, try opening them for saving.
    // If not or on failure, use FileDialogs to get paths.
    if(!currHeaderPath.isNull() && !currCodefilePath.isNull())
    {
        fileHeader.setFileName(currHeaderPath);
        fileCodefile.setFileName(currCodefilePath);
        if(!fileHeader.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            success = false;
            PRINTDBG(QString("Error opening file ") + currHeaderPath + ": " + fileHeader.errorString());
        }
        else if(!fileCodefile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            success = false;
            PRINTDBG(QString("Error opening file ") + currCodefilePath + ": " + fileCodefile.errorString());
        }
        else
        {
            success = true;
            headerPath = currHeaderPath;
            codefilePath = currCodefilePath;
        }
    }
    else
    {
        success = false; // Show save as dialogs.
    }

    if(!success)
    {
        success = true;
        if(fileDialog == NULL)
        {
            fileDialog = new FileDialog(parentWindow,
                                        QFileDialog::AcceptSave,
                                        QFileDialog::AnyFile);
        }

        headerPath = fileDialog->ShowGetHeader(currHeaderPath);
        if(headerPath.isNull() || headerPath.isEmpty())
        {
            emit results(OutResultUserCancelled, QString::null, QString::null);
            return;
        }

        fileHeader.setFileName(headerPath);
        if(!fileHeader.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            success = false;
        }
        else
        {
            codefilePath = fileDialog->ShowGetCodefile(currCodefilePath);
            fileCodefile.setFileName(codefilePath);
            if(codefilePath.isNull() || codefilePath.isEmpty())
            {
                emit results(OutResultUserCancelled, QString::null, QString::null);
                return;
            }
            if(!fileCodefile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                success = false;
            }
        }
    }

    if(success)
    {
        QTextStream stmOut(&fileHeader);
        stmOut.setCodec("UTF-8");
        stmOut << headMessage << AllSettings::FormatHeaderOutput();

        if(stmOut.status() != QTextStream::Ok)
        {
            success = false;
        }
    }

    if(success)
    {
        QTextStream stmOut(&fileCodefile);
        stmOut.setCodec("UTF-8");
        stmOut << headMessage << AllSettings::FormatCodefileOutput();

        if(stmOut.status() != QTextStream::Ok)
        {
            success = false;
        }
    }

    if(success)
    {
        emit results(OutResultSuccess, headerPath, codefilePath);
    }
    else
    {
        emit results(OutResultFileError, QString::null, QString::null);
    }

}

void Output::errorDialog_results(ErrorDialog::Result r)
{
    switch(r)
    {
        case ErrorDialog::EdResultCancel:
            emit results(OutResultUserCancelled, QString::null, QString::null);
            isSaving = false;
            break;

        case ErrorDialog::EdResultOk:
            if(isSaving)
            {
                isSaving = false;
                emit results(OutResultInvalid, QString::null, QString::null);
            }
            else
            {
                emit results(OutResultInfoDismissed, QString::null, QString::null);
            }
            break;

        case ErrorDialog::EdResultContinue:
            doOutput();
            isSaving = false;
            break;
    }
}
