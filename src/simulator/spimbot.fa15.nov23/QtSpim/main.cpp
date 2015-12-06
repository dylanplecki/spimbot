/* SPIM S20 MIPS simulator.
   Terminal interface for SPIM simulator.

   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <QtGui/QApplication>
#include <QTimer>
#include "spimview.h"
#include "../CPU/vasm.h"
#include "../SpimBOT/spimbotview.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep(x) (Sleep((x)*1000)) // Sleep is in terms of milliseconds
#endif

void parseCommandLine(QStringList args);
void run_spimbot();

SpimView* Window;
SpimbotView* MapWindow;
QApplication* App;

QStringList fileNames;
QStringList fileNames2;
QStringList fileArgs;

reg_image_t reg_images[2];

extern int yydebug;

//spimBOT stuff
extern int spimbot_cycle_limit;
extern bool spimbot_debug;
extern bool spimbot_exit_when_done;
extern bool spimbot_grading;
extern bool spimbot_largemap;
extern bool spimbot_maponly;
extern bool spimbot_nomap;
extern bool spimbot_randommap;
extern int spimbot_randomseed;
extern bool spimbot_run;
extern bool spimbot_testing;
extern bool spimbot_tournament;
extern bool spimbot_fancy;
extern bool multi_quad;

extern int quadrant_num;
extern int invisible_bot;
extern int quadmode_num;

// local stuff
static bool spim_vasm = false;
static QString vasm_files_prefix;

static int map_x = -1;
static int map_y = -1;

char* prof_file_name = NULL;

const char* stylesheet =
        "  QTabWidget::pane { /* The tab widget frame */"
        "         border-top: 2px solid #C2C7CB;"
        "     }"
        ""
        "     QTabWidget::tab-bar {"
        "         left: 5px; /* move to the right by 5px */RunAction.connect"
        "     }"
        ""
        "     /* Style the tab using the tab sub-control. Note that"
        "         it reads QTabBar _not_ QTabWidget */"
        "     QTabBar::tab {"
        "         background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                     stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,"
        "                                     stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);"
        "         border: 2px solid #C4C4C3;"
        "         border-bottom-color: #C2C7CB; /* same as the pane color */"
        "         border-top-left-radius: 4px;"
        "         border-top-right-radius: 4px;"
#ifdef _WIN32
        "         min-width: 32ex;"
        "         padding: 2px;"
#else
        "         min-width: 8ex;"
        "         padding: 2px; padding-left:10px; padding-right:10px;"
#endif
        "         font: bold 12px"
        "     }"
        ""
        "     QTabBar::tab:selected, QTabBar::tab:hover {"
        "         background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                     stop: 0 #fafafa, stop: 0.4 #f4f4f4,"
        "                                     stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);"
        "     }"
        ""
        "     QTabBar::tab:selected {"
        "         border: 2px solid #FF9933;"
        "         border-bottom-color: #C2C7CB; /* same as pane color */"
        "     }"
        ""
        "     QTabBar::tab:!selected {"
        "         margin-top: 2px; /* make non-selected tabs look smaller */"
        "     }";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    App = &a;
    SpimView win;
    Window = &win;
    SpimbotView mapWin;
    MapWindow = &mapWin;

    // Initialize Spim
    message_out.i = 1;
    console_out.i = 2;

    parseCommandLine(a.arguments());

    if (spimbot_grading || spimbot_testing) {
        spimbot_maponly = true;
        spimbot_nomap = true;
        spimbot_run = true;
        spimbot_exit_when_done = true;
        Window->SpimConsole->writeToStdout = true;
    }

    if (spim_vasm)
    {
        Window->SetExceptionHandler("", false);
        spimbot_maponly = true;
        spimbot_nomap = true;
    }

    // initialize before displaying anything to avoid uninitialized variables
    win.sim_ReinitializeSimulator();

    if (!spimbot_maponly) {
        win.SpimConsole->show();
        win.show();
    }

#ifndef QTSPIM
    if (!spimbot_nomap) {
        if (map_x != -1) {
            mapWin.move(map_x, map_y);
        }
        mapWin.show();
    }
#endif

    if (fileNames2.length() > 0)
    {
        num_contexts = 2;
        set_current_image(1);
        Window->InitializeWorld(1, true);
        // FIXME: figure out proper name
        strcpy(robots[1].name, fileNames2[0].toAscii().data());
        for (int i = 0; i < fileNames2.length(); ++i) {
            read_assembly_file(fileNames2[i].toAscii().data(), NULL);
        }
        set_current_image(0);
    }

    Window->InitializeWorld(0, num_contexts == 1);
    if (fileNames.length() > 0)
    {
        // FIXME: figure out proper name
        strcpy(robots[0].name, fileNames[0].toAscii().data());
        for (int i = 0; i < fileNames.length(); ++i) {
            read_assembly_file(fileNames[i].toAscii().data(), NULL);
        }
    }

    QString argsStr = fileArgs.join(" ");
    Window->SetCommandLine(argsStr);

    if (spim_vasm)
    {
        vasm(vasm_files_prefix.toStdString());
        return 0;
    }

    win.DisplayTextSegments();
    win.UpdateDataDisplay();

    a.setStyleSheet(stylesheet);

    if (spimbot_run) {
        QTimer::singleShot(0, Window, SLOT(sim_Run()));
    }

    return a.exec();
}


void parseCommandLine(QStringList args)
{
    for (int i = 1; i < args.length(); i++)
    {
#ifdef WIN32
        if (args[i][0] == '/')
        {
            args[i][0] = '-';
        }
#endif
        if ((args[i] == "-asm") || (args[i] == "-a"))
        {
            bare_machine = false;
            delayed_branches = false;
            delayed_loads = false;
        }
        else if ((args[i] == "-bare") || (args[i] == "-b"))
        {
            bare_machine = true;
            delayed_branches = true;
            delayed_loads = true;
            quiet = true;
        }
        else if ((args[i] == "-delayed_branches") || (args[i] == "-db"))
        {
            delayed_branches = true;
        }
        else if ((args[i] == "-delayed_loads") || (args[i] == "-dl"))
        {
            delayed_loads = true;
        }
        else if ((args[i] == "-exception") || (args[i] == "-e"))
        {
            Window->SetExceptionHandler("", true);
        }
        else if ((args[i] == "-noexception") || (args[i] == "-ne"))
        {
            Window->SetExceptionHandler("", false);
        }
        else if ((args[i] == "-exception_file") || (args[i] == "-ef"))
        {
            Window->SetExceptionHandler(args[++i], true);
        }
        else if ((args[i] == "-mapped_io") || (args[i] == "-mio"))
        {
            mapped_io = true;
        }
        else if ((args[i] == "-nomapped_io") || (args[i] == "-nmio"))
        {
            mapped_io = false;
        }
        else if ((args[i] == "-pseudo") || (args[i] == "-p"))
        {
            accept_pseudo_insts = true;
        }
        else if ((args[i] == "-nopseudo") || (args[i] == "-np"))
        {
            accept_pseudo_insts = false;
        }
        else if ((args[i] == "-quiet") || (args[i] == "-q"))
        {
            quiet = true;
        }
        else if ((args[i] == "-noquiet") || (args[i] == "-nq"))
        {
            quiet = false;
        }
        else if ((args[i] == "-trap") || (args[i] == "-t"))
        {
            Window->SetExceptionHandler("", true);
        }
        else if ((args[i] == "-notrap") || (args[i] == "-nt"))
        {
            Window->SetExceptionHandler("", false);
        }
        else if ((args[i] == "-trap_file") || (args[i] == "-tf"))
        {
            Window->SetExceptionHandler(args[++i], true);
        }
        else if ((args[i] == "-stext") || (args[i] == "-st"))
        {
            initial_text_size = args[++i].toInt();
        }
        else if ((args[i] == "-sdata") || (args[i] == "-sd"))
        {
            initial_data_size = args[++i].toInt();
        }
        else if ((args[i] == "-ldata") || (args[i] == "-ld"))
        {
            initial_data_limit = args[++i].toInt();
        }
        else if ((args[i] == "-sstack") || (args[i] == "-ss"))
        {
            initial_stack_size = args[++i].toInt();
        }
        else if ((args[i] == "-lstack") || (args[i] == "-ls"))
        {
            initial_stack_limit = args[++i].toInt();
        }
        else if ((args[i] == "-sktext") || (args[i] == "-skt"))
        {
            initial_k_text_size = args[++i].toInt();
        }
        else if ((args[i] == "-skdata") || (args[i] == "-skd"))
        {
            initial_k_data_size = args[++i].toInt();
        }
        else if ((args[i] == "-lkdata") || (args[i] == "-lkd"))
        {
            initial_k_data_limit = args[++i].toInt();
        }
        else if (((args[i] == "-file") || (args[i] == "-f")))
        {
            while (i + 1 < args.length() && args[i + 1][0] != '-')
            {
                fileNames.append(args[++i]);
            }
        }
        else if (((args[i] == "-file2") || (args[i] == "-f2")))
        {
            while (i + 1 < args.length() && args[i + 1][0] != '-')
            {
                fileNames2.append(args[++i]);
            }
        }
        else if (args[i] == "-args")
        {
            while (i + 1 < args.length() && args[i + 1] != "--")
            {
                fileArgs.append(args[++i]);
            }
            ++i;
        }
        else if ((args[i] == "-prof_file") && (i + 1 < args.length()))
        {
            prof_file_name = new char[80];
            strcpy(prof_file_name, args[++i].toAscii().data());
        }
        else if (args[i] == "-debug")
        {
            spimbot_debug = true;
        }
        else if (args[i] == "-exit_when_done")
        {
            spimbot_exit_when_done = true;
        }
        else if (args[i] == "-grading")
        {
            spimbot_grading = true;
        }
        else if (args[i] == "-largemap")
        {
            //TODO Resize window in a nicer way
            spimbot_largemap = true;
            Window->resize(700, 700);
            MapWindow->resize(700, 700);
        }
        else if (args[i] == "-maponly")
        {
            spimbot_maponly = true;
        }
        else if (args[i] == "-nomap")
        {
            spimbot_nomap = true;
        }
        else if (args[i] == "-randommap")
        {
            spimbot_randommap = true;
        }
        else if ((args[i] == "-randomseed") && (i + 1 < args.length()))
        {
            bool ok;
            spimbot_randomseed = args[++i].toInt(&ok, 10);
            if(!ok)
                spimbot_randomseed = -1;
        }
        else if (args[i] == "-run")
        {
            spimbot_run = true;
        }
        else if (args[i] == "-testing")
        {
            spimbot_testing = true;
        }
        else if (args[i] == "-invisible")
        {
            if (i + 1 < args.length() && args[i + 1][0] != '-')
            {
                invisible_bot = args[++i].toInt();
            }
        }

        else if (args[i] == "-tournament")
        {
            spimbot_tournament = true;
        }
        else if(args[i] == "-limit" && i + 1 < args.length())
        {
            bool ok;
            spimbot_cycle_limit = args[++i].toInt(&ok, 10);
        }
        else if (args[i] == "-stdout")
        {
            Window->SpimConsole->writeToStdout = true;
        }
        else if (args[i] == "-stderr")
        {
            Window->SpimConsole->writeToStderr = true;
        }
        else if (args[i] == "-updatedata")
        {
            Window->updateDataSegments = true;
        }
        else if (args[i] == "-noupdatedata")
        {
            Window->updateDataSegments = false;
        }
        else if ((args[i] == "-drawcycles") && (i + 1 < args.length()))
        {
            repaint_cycles = args[++i].toInt();
        }
        else if ((args[i] == "-debug_parser"))
        {
            yydebug = 1;
        }
        else if (args[i] == "-vasm" && i + 1 < args.length())
        {
            spim_vasm = true;
            vasm_files_prefix = args[++i];
        }
        else if ((args[i] == "-fancy"))
        {
            spimbot_fancy = true;
        }
        else if (args[i] == "-map_pos" && i + 2 < args.length())
        {
            map_x = args[++i].toInt();
            map_y = args[++i].toInt();
        }
        else
        {
            error ("\nUnknown argument: %s (ignored)\n", args[i].toLocal8Bit().data());
            error ("Usage: spim\n\
-bare                   Bare machine (no pseudo-ops, delayed branches and loads)\n\
-asm                    Extended machine (pseudo-ops, no delayed branches and loads) (default)\n\
-delayed_branches       Execute delayed branches\n\
-delayed_loads          Execute delayed loads\n\
-exception              Load exception handler (default)\n\
-noexception            Do not load exception handler\n\
-exception_file <file>  Specify exception handler in place of default\n\
-quiet                  Do not print warnings\n\
-noquiet                Print warnings (default)\n\
-mapped_io              Enable memory-mapped IO\n\
-nomapped_io            Do not enable memory-mapped IO (default)\n\
-dust_quadrant          Specifies quandrant where dust is initialized\n\
-quadrant_modes         Specifies type of dust distribution (takes single digit)\n\
-multi_quadrants        Enable multiple quadrant initialization of dust\n\
-file <file> <args>     Assembly code file and arguments to program\n");
        }
    }
}
