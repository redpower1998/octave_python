////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011-2020 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////

#if ! defined (octave_resource_manager_h)
#define octave_resource_manager_h 1

#include <QComboBox>
#include <QIcon>
#include <QPointer>
#include <QTranslator>
#include <QTemporaryFile>

#include "gui-settings.h"

namespace octave
{
  class resource_manager : public QObject
  {
    Q_OBJECT

  protected:

  public:

    resource_manager (void);

    // No copying!

    resource_manager (const resource_manager&) = delete;

    resource_manager& operator = (const resource_manager&) = delete;

    ~resource_manager ();

    QString get_gui_translation_dir (void);

    void config_translators (QTranslator *qt_tr, QTranslator *qsci_tr,
                             QTranslator *gui_tr);

    gui_settings * get_settings (void) const;

    gui_settings * get_default_settings (void) const;

    QString get_settings_directory (void);

    QString get_settings_file (void);

    QString get_default_font_family (void);

    QPointer<QTemporaryFile>
    create_tmp_file (const QString& extension = QString (),
                     const QString& contents = QString ());

    void remove_tmp_file (QPointer<QTemporaryFile> tmp_file);

    void reload_settings (void);

    void set_settings (const QString& file);

    bool update_settings_key (const QString& new_key, const QString& old_key);

    bool is_first_run (void) const;

    void update_network_settings (void);

    QIcon icon (const QString& icon_name, bool fallback = true);

    void get_codecs (QStringList *codecs);

    void combo_encoding (QComboBox *combo, const QString& current = QString ());

  private:

    QString m_settings_directory;

    QString m_settings_file;

    gui_settings *m_settings;

    gui_settings *m_default_settings;

    QList<QTemporaryFile *> m_temporary_files;
  };
}

#endif