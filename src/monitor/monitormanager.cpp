/***************************************************************************
 *   Copyright (C) 2007 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include "monitormanager.h"
#include "core.h"
#include "renderer.h"
#include "kdenlivesettings.h"
#include "mainwindow.h"
#include "doc/kdenlivedoc.h"

#include <mlt++/Mlt.h>

#include <QObject>
#include <QTimer>
#include <KDebug>


MonitorManager::MonitorManager(QObject *parent) :
        QObject(parent),
        m_document(NULL),
        m_clipMonitor(NULL),
        m_projectMonitor(NULL),
        m_activeMonitor(NULL)
{
    setupActions();
}

Timecode MonitorManager::timecode() const
{
    return m_timecode;
}

void MonitorManager::setDocument(KdenliveDoc *doc)
{
    m_document = doc;
}

void MonitorManager::initMonitors(Monitor *clipMonitor, Monitor *projectMonitor, RecMonitor *recMonitor)
{
    m_clipMonitor = clipMonitor;
    m_projectMonitor = projectMonitor;
    
    connect(m_clipMonitor->render, SIGNAL(activateMonitor(Kdenlive::MonitorId)), this, SLOT(activateMonitor(Kdenlive::MonitorId)));
    connect(m_projectMonitor->render, SIGNAL(activateMonitor(Kdenlive::MonitorId)), this, SLOT(activateMonitor(Kdenlive::MonitorId)));

    m_monitorsList.append(clipMonitor);
    m_monitorsList.append(projectMonitor);
    if (recMonitor)
        m_monitorsList.append(recMonitor);
}

void MonitorManager::appendMonitor(AbstractMonitor *monitor)
{
    if (!m_monitorsList.contains(monitor)) m_monitorsList.append(monitor);
}

void MonitorManager::removeMonitor(AbstractMonitor *monitor)
{
    m_monitorsList.removeAll(monitor);
}

AbstractMonitor* MonitorManager::monitor(Kdenlive::MonitorId monitorName)
{
    AbstractMonitor *monitor = NULL;
    for (int i = 0; i < m_monitorsList.size(); ++i) {
        if (m_monitorsList[i]->id() == monitorName) {
        monitor = m_monitorsList.at(i);
	}
    }
    return monitor;
}

void MonitorManager::setConsumerProperty(const QString &name, const QString &value)
{
    if (m_clipMonitor) m_clipMonitor->render->setConsumerProperty(name, value);
    if (m_projectMonitor) m_projectMonitor->render->setConsumerProperty(name, value);
}

bool MonitorManager::activateMonitor(Kdenlive::MonitorId name, bool forceRefresh)
{
    if (m_clipMonitor == NULL || m_projectMonitor == NULL)
        return false;
    if (m_activeMonitor && m_activeMonitor->id() == name) {
	if (forceRefresh) m_activeMonitor->start();
        return false;
    }
    m_activeMonitor = NULL;
    for (int i = 0; i < m_monitorsList.count(); ++i) {
        if (m_monitorsList.at(i)->id() == name) {
            m_activeMonitor = m_monitorsList.at(i);
        }
        else m_monitorsList.at(i)->stop();
    }
    if (m_activeMonitor) {
        m_activeMonitor->blockSignals(true);
        m_activeMonitor->parentWidget()->raise();
	m_activeMonitor->blockSignals(false);
        m_activeMonitor->start();
        
    }
    emit checkColorScopes();
    return (m_activeMonitor != NULL);
}

bool MonitorManager::isActive(Kdenlive::MonitorId id) const
{
    return m_activeMonitor ? m_activeMonitor->id() == id: false;
}

void MonitorManager::slotSwitchMonitors(bool activateClip)
{
    if (activateClip)
        activateMonitor(Kdenlive::ClipMonitor);
    else
        activateMonitor(Kdenlive::ProjectMonitor);
}

void MonitorManager::stopActiveMonitor()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->pause();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->pause();
}

void MonitorManager::slotPlay()
{
    if (m_activeMonitor) m_activeMonitor->slotPlay();
}

void MonitorManager::slotPause()
{
    stopActiveMonitor();
}

void MonitorManager::slotPlayZone()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotPlayZone();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotPlayZone();
}

void MonitorManager::slotLoopZone()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotLoopZone();
    else m_projectMonitor->slotLoopZone();
}

void MonitorManager::slotRewind(double speed)
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotRewind(speed);
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotRewind(speed);
}

void MonitorManager::slotForward(double speed)
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotForward(speed);
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotForward(speed);
}

void MonitorManager::slotRewindOneFrame()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotRewindOneFrame();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotRewindOneFrame();
}

void MonitorManager::slotForwardOneFrame()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotForwardOneFrame();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotForwardOneFrame();
}

void MonitorManager::slotRewindOneSecond()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotRewindOneFrame(m_timecode.fps());
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotRewindOneFrame(m_timecode.fps());
}

void MonitorManager::slotForwardOneSecond()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotForwardOneFrame(m_timecode.fps());
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotForwardOneFrame(m_timecode.fps());
}

void MonitorManager::slotStart()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotStart();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotStart();
}

void MonitorManager::slotEnd()
{
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->slotEnd();
    else if (m_activeMonitor == m_projectMonitor) m_projectMonitor->slotEnd();
}

void MonitorManager::resetProfiles(const Timecode &tc)
{
    m_timecode = tc;
    slotResetProfiles();
    //QTimer::singleShot(300, this, SLOT(slotResetProfiles()));
}

void MonitorManager::slotResetProfiles()
{
    if (m_projectMonitor == NULL || m_clipMonitor == NULL) {
	return;
    }
    blockSignals(true);
    Kdenlive::MonitorId active = m_activeMonitor ? m_activeMonitor->id() : Kdenlive::NoMonitor;
    m_clipMonitor->resetProfile(KdenliveSettings::current_profile());
    m_projectMonitor->resetProfile(KdenliveSettings::current_profile());
    if (active != Kdenlive::NoMonitor) activateMonitor(active);
    blockSignals(false);
    if (m_activeMonitor) m_activeMonitor->parentWidget()->raise();
    emit checkColorScopes();
}

void MonitorManager::slotRefreshCurrentMonitor(const QString &id)
{
    // Clip producer was modified, check if clip is currently displayed in clip monitor
    m_clipMonitor->reloadProducer(id);
    if (m_activeMonitor == m_clipMonitor) m_clipMonitor->refreshMonitor();
    else m_projectMonitor->refreshMonitor();
}

void MonitorManager::slotUpdateAudioMonitoring()
{
    // if(...) added since they are 0x0 when the config wizard is running! --Granjow
    /*if (m_clipMonitor) {
        m_clipMonitor->render->analyseAudio = KdenliveSettings::monitor_audio();
    }
    if (m_projectMonitor) {
        m_projectMonitor->render->analyseAudio = KdenliveSettings::monitor_audio();
    }*/
    for (int i = 0; i < m_monitorsList.count(); ++i) {
        if (m_monitorsList.at(i)->abstractRender()) m_monitorsList.at(i)->abstractRender()->analyseAudio = KdenliveSettings::monitor_audio();
    }
}

void MonitorManager::clearScopeSource()
{
    emit clearScopes();
}

void MonitorManager::updateScopeSource()
{
    emit checkColorScopes();
}

AbstractRender *MonitorManager::activeRenderer()
{
    if (m_activeMonitor) {
        return m_activeMonitor->abstractRender();
    }
    return NULL;
}

void MonitorManager::slotSwitchFullscreen()
{
    if (m_activeMonitor) m_activeMonitor->slotSwitchFullScreen();
}

QString MonitorManager::getProjectFolder() const
{
    if (m_document == NULL) {
	kDebug()<<" + + +NULL DOC!!";
	return QString();
    }
    return m_document->projectFolder().path(KUrl::AddTrailingSlash);
}

void MonitorManager::setupActions()
{
    KAction* monitorPlay = new KAction(KIcon("media-playback-start"), i18n("Play"), this);
    monitorPlay->setShortcut(Qt::Key_Space);
    pCore->window()->addAction("monitor_play", monitorPlay);
    connect(monitorPlay, SIGNAL(triggered(bool)), SLOT(slotPlay()));

    KAction* monitorPause = new KAction(KIcon("media-playback-stop"), i18n("Pause"), this);
    monitorPause->setShortcut(Qt::Key_K);
    pCore->window()->addAction("monitor_pause", monitorPause);
    connect(monitorPause, SIGNAL(triggered(bool)), SLOT(slotPause()));

    // TODO: port later when we are able to setup the monitor menus from this class
//     m_playZone = new KAction(KIcon("media-playback-start"), i18n("Play Zone"), this);
//     m_playZone->setShortcut(Qt::CTRL + Qt::Key_Space);
//     pCore->window()->addAction("monitor_play_zone", m_playZone);
//     connect(m_playZone, SIGNAL(triggered(bool)), SLOT(slotPlayZone()));
// 
//     m_loopZone = new KAction(KIcon("media-playback-start"), i18n("Loop Zone"), this);
//     m_loopZone->setShortcut(Qt::ALT + Qt::Key_Space);
//     pCore->window()->addAction("monitor_loop_zone", m_loopZone);
//     connect(m_loopZone, SIGNAL(triggered(bool)), SLOT(slotLoopZone()));

    KAction *fullMonitor = new KAction(i18n("Switch monitor fullscreen"), this);
    fullMonitor->setIcon(KIcon("view-fullscreen"));
    pCore->window()->addAction("monitor_fullscreen", fullMonitor);
    connect(fullMonitor, SIGNAL(triggered(bool)), SLOT(slotSwitchFullscreen()));

    KAction* monitorSeekBackward = new KAction(KIcon("media-seek-backward"), i18n("Rewind"), this);
    monitorSeekBackward->setShortcut(Qt::Key_J);
    pCore->window()->addAction("monitor_seek_backward", monitorSeekBackward);
    connect(monitorSeekBackward, SIGNAL(triggered(bool)), SLOT(slotRewind()));

    KAction* monitorSeekBackwardOneFrame = new KAction(KIcon("media-skip-backward"), i18n("Rewind 1 Frame"), this);
    monitorSeekBackwardOneFrame->setShortcut(Qt::Key_Left);
    pCore->window()->addAction("monitor_seek_backward-one-frame", monitorSeekBackwardOneFrame);
    connect(monitorSeekBackwardOneFrame, SIGNAL(triggered(bool)), SLOT(slotRewindOneFrame()));

    KAction* monitorSeekBackwardOneSecond = new KAction(KIcon("media-skip-backward"), i18n("Rewind 1 Second"), this);
    monitorSeekBackwardOneSecond->setShortcut(Qt::SHIFT + Qt::Key_Left);
    pCore->window()->addAction("monitor_seek_backward-one-second", monitorSeekBackwardOneSecond);
    connect(monitorSeekBackwardOneSecond, SIGNAL(triggered(bool)), SLOT(slotRewindOneSecond()));

    KAction* monitorSeekForward = new KAction(KIcon("media-seek-forward"), i18n("Forward"), this);
    monitorSeekForward->setShortcut(Qt::Key_L);
    pCore->window()->addAction("monitor_seek_forward", monitorSeekForward);
    connect(monitorSeekForward, SIGNAL(triggered(bool)), SLOT(slotForward()));

    KAction* projectStart = new KAction(KIcon("go-first"), i18n("Go to Project Start"), this);
    projectStart->setShortcut(Qt::CTRL + Qt::Key_Home);
    pCore->window()->addAction("seek_start", projectStart);
    connect(projectStart, SIGNAL(triggered(bool)), SLOT(slotStart()));

    KAction* projectEnd = new KAction(KIcon("go-last"), i18n("Go to Project End"), this);
    projectEnd->setShortcut(Qt::CTRL + Qt::Key_End);
    pCore->window()->addAction("seek_end", projectEnd);
    connect(projectEnd, SIGNAL(triggered(bool)), SLOT(slotEnd()));

    KAction* monitorSeekForwardOneFrame = new KAction(KIcon("media-skip-forward"), i18n("Forward 1 Frame"), this);
    monitorSeekForwardOneFrame->setShortcut(Qt::Key_Right);
    pCore->window()->addAction("monitor_seek_forward-one-frame", monitorSeekForwardOneFrame);
    connect(monitorSeekForwardOneFrame, SIGNAL(triggered(bool)), SLOT(slotForwardOneFrame()));

    KAction* monitorSeekForwardOneSecond = new KAction(KIcon("media-skip-forward"), i18n("Forward 1 Second"), this);
    monitorSeekForwardOneSecond->setShortcut(Qt::SHIFT + Qt::Key_Right);
    pCore->window()->addAction("monitor_seek_forward-one-second", monitorSeekForwardOneSecond);
    connect(monitorSeekForwardOneSecond, SIGNAL(triggered(bool)), SLOT(slotForwardOneSecond()));

    KSelectAction *interlace = new KSelectAction(i18n("Deinterlacer"), this);
    interlace->addAction(i18n("One Field (fast)"));
    interlace->addAction(i18n("Linear Blend (fast)"));
    interlace->addAction(i18n("YADIF - temporal only (good)"));
    interlace->addAction(i18n("YADIF - temporal + spacial (best)"));
    if (KdenliveSettings::mltdeinterlacer() == "linearblend") interlace->setCurrentItem(1);
    else if (KdenliveSettings::mltdeinterlacer() == "yadif-temporal") interlace->setCurrentItem(2);
    else if (KdenliveSettings::mltdeinterlacer() == "yadif") interlace->setCurrentItem(3);
    else interlace->setCurrentItem(0);
    pCore->window()->addAction("mlt_interlace", interlace);
    connect(interlace, SIGNAL(triggered(int)), SLOT(slotSetDeinterlacer(int)));
    
    KSelectAction *interpol = new KSelectAction(i18n("Interpolation"), this);
    interpol->addAction(i18n("Nearest Neighbor (fast)"));
    interpol->addAction(i18n("Bilinear (good)"));
    interpol->addAction(i18n("Bicubic (better)"));
    interpol->addAction(i18n("Hyper/Lanczos (best)"));
    if (KdenliveSettings::mltinterpolation() == "bilinear") interpol->setCurrentItem(1);
    else if (KdenliveSettings::mltinterpolation() == "bicubic") interpol->setCurrentItem(2);
    else if (KdenliveSettings::mltinterpolation() == "hyper") interpol->setCurrentItem(3);
    else interpol->setCurrentItem(0);
    pCore->window()->addAction("mlt_interpolation", interpol);
    connect(interpol, SIGNAL(triggered(int)), SLOT(slotSetInterpolation(int)));

    KAction* zoneStart = new KAction(KIcon("media-seek-backward"), i18n("Go to Zone Start"), this);
    zoneStart->setShortcut(Qt::SHIFT + Qt::Key_I);
    pCore->window()->addAction("seek_zone_start", zoneStart);
    connect(zoneStart, SIGNAL(triggered(bool)), SLOT(slotZoneStart()));

    KAction* zoneEnd = new KAction(KIcon("media-seek-forward"), i18n("Go to Zone End"), this);
    zoneEnd->setShortcut(Qt::SHIFT + Qt::Key_O);
    pCore->window()->addAction("seek_zone_end", zoneEnd);
    connect(zoneEnd, SIGNAL(triggered(bool)), SLOT(slotZoneEnd()));

    KAction *markIn = new KAction(i18n("Set Zone In"), this);
    markIn->setShortcut(Qt::Key_I);
    pCore->window()-> addAction("mark_in", markIn);
    connect(markIn, SIGNAL(triggered(bool)), SLOT(slotSetInPoint()));

    KAction *markOut = new KAction(i18n("Set Zone Out"), this);
    markOut->setShortcut(Qt::Key_O);
    pCore->window()-> addAction("mark_out", markOut);
    connect(markOut, SIGNAL(triggered(bool)), SLOT(slotSetOutPoint()));

}


void MonitorManager::slotSetDeinterlacer(int ix)
{
    QString value;
    switch (ix) {

    case 1:
        value = "linearblend";
        break;
    case 2:
        value = "yadif-nospatial";
        break;
    case 3:
        value = "yadif";
        break;
    default:
        value = "onefield";
    }
    KdenliveSettings::setMltdeinterlacer(value);
    setConsumerProperty("deinterlace_method", value);
}

void MonitorManager::slotSetInterpolation(int ix)
{
    QString value;
    switch (ix) {
    case 1:
        value = "bilinear";
        break;
    case 2:
        value = "bicubic";
        break;
    case 3:
        value = "hyper";
        break;
    default:
        value = "nearest";
    }
    KdenliveSettings::setMltinterpolation(value);
    setConsumerProperty("rescale", value);
}

Monitor* MonitorManager::clipMonitor()
{
    return m_clipMonitor;
}

Monitor* MonitorManager::projectMonitor()
{
    return m_projectMonitor;
}

void MonitorManager::slotZoneStart()
{
    if (m_activeMonitor == m_clipMonitor)
        m_clipMonitor->slotZoneStart();
    else if (m_activeMonitor == m_projectMonitor)
        m_projectMonitor->slotZoneStart();
}

void MonitorManager::slotZoneEnd()
{
    if (m_activeMonitor == m_projectMonitor)
        m_projectMonitor->slotZoneEnd();
    else if (m_activeMonitor == m_clipMonitor)
        m_clipMonitor->slotZoneEnd();
}

void MonitorManager::slotSetInPoint()
{
    if (m_activeMonitor == m_clipMonitor) {
        m_clipMonitor->slotSetZoneStart();
    } else if (m_activeMonitor == m_projectMonitor) {
        m_projectMonitor->slotSetZoneStart();
    }
}

void MonitorManager::slotSetOutPoint()
{
    if (m_activeMonitor == m_clipMonitor) {
        m_clipMonitor->slotSetZoneEnd();
    } else if (m_activeMonitor == m_projectMonitor) {
        m_projectMonitor->slotSetZoneEnd();
    }
}

#include "monitormanager.moc"
