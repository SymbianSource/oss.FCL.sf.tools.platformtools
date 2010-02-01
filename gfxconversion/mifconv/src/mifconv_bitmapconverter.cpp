/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Mifconv bitmap converters class.
*
*/


#include "mifconv.h"
#include "mifconv_bitmapconverter.h"
#include "mifconv_util.h"
#include "mifconv_exception.h"
#include "mifconv_argumentmanager.h"
#include <stdio.h>

const MifConvString BMCONV_DEFAULT_PATH(EPOC_TOOLS_PATH);

/**
 *
 */
MifConvBitmapConverter::MifConvBitmapConverter()
{
    MifConvArgumentManager* argMgr = MifConvArgumentManager::Instance();
	// Output file:
	iTargetFilename = MifConvUtil::FilenameWithoutExtension(argMgr->TargetFile()) + "." + MifConvString(MBM_FILE_EXTENSION);
}

/**
 *
 */
MifConvBitmapConverter::~MifConvBitmapConverter()
{
}

/**
 *
 */
void MifConvBitmapConverter::Init()
{
    CleanupTargetFiles();
}

/**
 *
 */
void MifConvBitmapConverter::CleanupTargetFiles()
{
	if( MifConvUtil::FileExists(iTargetFilename) )
	{
        // Try to remove file MIFCONV_MAX_REMOVE_TRIES times, no exception in case of failure:
        MifConvUtil::RemoveFile(iTargetFilename, MIFCONV_MAX_REMOVE_TRIES, true);
	}
}

/**
 *
 */
void MifConvBitmapConverter::AppendFile( const MifConvSourceFile& sourcefile )
{    
	if( MifConvUtil::FileExtension( sourcefile.Filename() ) == BMP_FILE_EXTENSION )
	{
		iSourceFiles.push_back( sourcefile );
	}
}

/**
 *
 */
void MifConvBitmapConverter::Convert()
{
    if( iSourceFiles.size() > 0 )
    {
	    ConvertToMbm();
    }
}

/**
 *
 */
void MifConvBitmapConverter::Cleanup(bool err)
{
	CleanupTempFiles();
	if( err )
	{
	    CleanupTargetFiles();
	}
}

/**
 *
 */
void MifConvBitmapConverter::ConvertToMbm()
{    
    RunBmconv();
}

/**
 *
 */
void MifConvBitmapConverter::InitTempFile()
{
    MifConvArgumentManager* argMgr = MifConvArgumentManager::Instance();
    // Construct temp file name
    iTempDir = MifConvUtil::DefaultTempDirectory();
    const MifConvString& tempDirArg = argMgr->StringValue(MifConvTempPathArg);
    if( tempDirArg.length() > 0 )
    {
        iTempDir = tempDirArg;
    }

    if( iTempDir.length() > 0 && iTempDir.at(iTempDir.length()-1) != DIR_SEPARATOR2 )
    {
        iTempDir.append(DIR_SEPARATOR);
    }

    // Generate new temp-filename:
    iTempDir.append(MifConvUtil::TemporaryFilename());

    // append tmp at as postfix
    // this is needed because the generated name can contain a single period '.'
    // character as the last character which is eaten away when the directory created.
    iTempDir.append(MifConvString("tmp"));

    MifConvUtil::EnsurePathExists(iTempDir);

    iTempDir.append(DIR_SEPARATOR);

    iTempFilename = iTempDir + MifConvUtil::FilenameWithoutExtension(MifConvUtil::FilenameWithoutPath(argMgr->TargetFile()));
    iTempFilename += BMCONV_TEMP_FILE_POSTFIX;

    // Create temp file
    fstream tempFile(iTempFilename.c_str(), ios::out|ios::binary|ios::trunc);
    if (!tempFile.is_open())
    {        
        throw MifConvException(MifConvString("Unable to create tmp file! ") + iTempFilename);        
    }

    try {
        // quiet mode        
        tempFile << BMCONV_OPTION_PREFIX << BMCONV_QUIET_PARAMETER << " ";
        // Palette argument
        const MifConvString& paletteArg = argMgr->StringValue(MifConvPaletteFileArg);
        if( paletteArg.length() > 0 )
        {
            tempFile << BMCONV_OPTION_PREFIX << BMCONV_PALETTE_PARAMETER;            
            tempFile << MifConvString(paletteArg + " ");
        }

        tempFile << iTargetFilename << " ";                
        // Add filenames to the temp file
        for( MifConvSourceFileList::iterator i = iSourceFiles.begin(); i != iSourceFiles.end(); ++i )
        {
            AppendBmpToTempFile(tempFile, *i);
        }
    }
    catch(...) {
        tempFile.close();
        throw;
    }        

    tempFile.close();
}

/**
 *
 */
void MifConvBitmapConverter::RunBmconv()
{
    MifConvArgumentManager* argMgr = MifConvArgumentManager::Instance();
    // Create and initialize the temp file:    
    InitTempFile();

    // Build bmconv command    
    MifConvString bmconvCommand("\""); // Open " mark
    
    const MifConvString& bmconvPath = argMgr->StringValue(MifConvBmconvPathArg);
    const MifConvString& defaultBmconvPath = GetDefaultBmConvPath();
    if( bmconvPath.length() > 0 )
    {
        bmconvCommand += bmconvPath; // If the path is given, use it.
    }
    else
    {
        bmconvCommand += defaultBmconvPath; // Use default path
    }

    // Ensure that the last char of the path is dir-separator:
    if( bmconvCommand.length() > 1 && bmconvCommand.at(bmconvCommand.length()-1) != DIR_SEPARATOR2 )
        bmconvCommand += DIR_SEPARATOR;

    // Then add bmconv executable call and close the " mark
    bmconvCommand += BMCONV_EXECUTABLE_NAME + MifConvString("\" ");  
    bmconvCommand += "\"" + iTempFilename + "\"";
        
    MifConvUtil::EnsurePathExists(iTargetFilename, true);
    
    cout << "Writing mbm: " << iTargetFilename << endl;           
    int err = 0;
    
#ifdef __linux__
    if ((err = system (MifConvString(bmconvCommand).c_str())) != 0)   // Returns 0 if success
#else
    if ((err = system (MifConvString("\""+bmconvCommand+"\"").c_str())) != 0)   // Returns 0 if success
#endif
    {
    	THROW_ERROR_COMMON("Executing BMCONV failed", MifConvString(__FILE__), __LINE__);
    }
}

/**
 *
 */
void MifConvBitmapConverter::CleanupTempFiles()
{
    if( iTempFilename.length() > 0 && remove( iTempFilename.c_str() ) != 0 )
    {
        perror( "Error deleting temporary file (bitmap conversion)" );
    }
    
    if( iTempDir.length() > 0 && MifConvUtil::RemoveDirectory( iTempDir ) != 0 )
    {
        perror( "Error deleting temporary directory (bitmap conversion)" );
    }
}

/**
 *
 */
const MifConvString& MifConvBitmapConverter::GetDefaultBmConvPath()
{
    if( iDefaultBmConvPath.length() == 0 )
    {        
        // Check if the EPOCROOT is given
        MifConvString epocRoot(MifConvArgumentManager::Instance()->EpocRoot());
        if( epocRoot.length() > 0 )
        {
            // EPOCROOT environment variable defined.
            iDefaultBmConvPath = epocRoot + BMCONV_DEFAULT_PATH;
        }        
    }

    return iDefaultBmConvPath;
}

/**
 *
 */
void MifConvBitmapConverter::AppendBmpToTempFile(fstream& aStream, const MifConvSourceFile& bmpFile)
    {
    cout << "Loading file: " << bmpFile.Filename() << endl;

    aStream << BMCONV_OPTION_PREFIX;
    aStream << bmpFile.DepthString();
    aStream << bmpFile.Filename();
    aStream << " ";
        
    // Prepare also for the case that mask is not used at all.
    const MifConvString& maskName = bmpFile.BmpMaskFilename();
    if (maskName.length() > 0 )
    {
        cout << "Loading file: " << maskName << endl;
        aStream << BMCONV_OPTION_PREFIX;
        aStream << bmpFile.MaskDepthString();
        aStream << maskName;        
    }
    aStream << " ";    
    }
