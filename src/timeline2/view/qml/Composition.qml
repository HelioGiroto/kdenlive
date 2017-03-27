/***************************************************************************
 *   Copyright (C) 2017 by Jean-Baptiste Mardelle                          *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *   Based on work by Dan Dennedy <dan@dennedy.org> (Shotcut)              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.0
import QtGraphicalEffects 1.0
import QtQml.Models 2.2
import QtQuick.Window 2.2

Rectangle {
    id: compositionRoot
    property real timeScale: 1.0
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property int modelStart: x
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isBlank: false
    property bool isAudio: false
    property bool isComposition: true
    property bool grouped: false
    property int binId: 0
    property int trackHeight
    property int trackIndex //Index in track repeater
    property int trackId: -42    //Id in the model
    property int a_track: -1
    property int clipId     //Id of the clip in the model
    property int originalTrackId: trackId
    property int originalX: x
    property int originalDuration: clipDuration
    property int lastValidDuration: clipDuration
    property int draggedX: x
    property int a_trackPos: -1
    property bool selected: false
    property double speed: 1.0
    property color borderColor: 'black'

    signal clicked(var clip, int shiftClick)
    signal moved(var clip)
    signal dragged(var clip, var mouse)
    signal dropped(var clip)
    signal draggedToTrack(var clip, int pos)
    signal trimmingIn(var clip, real newDuration, var mouse)
    signal trimmedIn(var clip)
    signal trimmingOut(var clip, real newDuration, var mouse)
    signal trimmedOut(var clip)

    onModelStartChanged: {
        console.log("MODEL START CHANGED !!!!!!", modelStart, clipId, x);
        x = modelStart * timeScale;
    }
    onTimeScaleChanged: {
        console.log("SCALE CHANGED !!!!!!", timeScale)
        x = modelStart * timeScale;
        width = clipDuration * timeScale;
    }
    onTrackHeightChanged: {
        a_trackPos = root.getTrackYFromId(a_track) - mapToItem(trackRoot, 0, 0).y - trackRoot.mapToItem(null, 0, 0).y + ruler.height
    }

    SystemPalette { id: activePalette }
    color: Qt.darker(getColor())

    border.color: selected? 'red' : borderColor
    border.width: 1.5
    clip: false
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction
    opacity: Drag.active? 0.5 : 1.0

    function getColor() {
        if (mltService === 'color') {
            //console.log('clip color', clipResource, " / ", '#' + clipResource.substring(2, 8))
            if (clipResource.length == 10) {
                // 0xRRGGBBAA
                return '#' + clipResource.substring(2, 8)
            }
        }
        return 'mediumpurple'
        //root.shotcutBlue
    }

    function reparent(track) {
        parent = track
        isAudio = track.isAudio
        height = track.height / 2
        y = track.y + track.height / 2
        generateWaveform()
    }


    Rectangle {
        // text background
        color: 'lightgray'
        opacity: 0.7
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.border.width
        anchors.leftMargin: parent.border.width
            // + ((isAudio || !settings.timelineShowThumbnails) ? 0 : inThumbnail.width)
        width: Math.min(label.width + 2, parent.width - 2)
        height: label.height
    }

    Text {
        id: label
        text: clipName
        font.pixelSize: root.baseUnit
        width: parent.width - 2
        clip: true
        anchors {
            top: parent.top
            left: parent.left
            topMargin: parent.border.width + 1
            leftMargin: parent.border.width + 1
                // + ((isAudio || !settings.timelineShowThumbnails) ? 0 : inThumbnail.width) + 1
        }
        color: 'black'
    }

    // target track
    Rectangle {
        width: parent.width
        height: 8
        gradient: Gradient {
            GradientStop { position: 0.0; color: "mediumpurple" }
            GradientStop { position: 1.0; color: "#00000000" }
        }
        y: a_trackPos
    }

    onA_trackChanged: {
        a_trackPos = root.getTrackYFromId(a_track) - mapToItem(trackRoot, 0, 0).y - trackRoot.mapToItem(null, 0, 0).y + ruler.height
    }

    states: [
        State {
            name: 'normal'
            when: !compositionRoot.selected
            PropertyChanges {
                target: compositionRoot
                z: 0
            }
        },
        State {
            name: 'selectedBlank'
            when: compositionRoot.selected && compositionRoot.isBlank
            PropertyChanges {
                target: compositionRoot
                color: Qt.darker(getColor())
            }
        },
        State {
            name: 'selected'
            when: compositionRoot.selected
            PropertyChanges {
                target: compositionRoot
                z: 1
                color: getColor()
            }
        }
    ]

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !isBlank
        acceptedButtons: Qt.LeftButton
        drag.target: parent
        drag.axis: Drag.XAxis
        property int startX

        onPressed: {
            root.stopScrolling = true
            originalX = parent.x
            originalTrackId = compositionRoot.trackId
            startX = parent.x
            compositionRoot.forceActiveFocus();
            compositionRoot.clicked(compositionRoot, mouse.modifiers === Qt.ShiftModifier)
        }
        onPositionChanged: {
            if (mouse.y < 0 || mouse.y > height) {
                parent.draggedToTrack(compositionRoot, mapToItem(null, 0, mouse.y).y)
            } else {
                parent.dragged(compositionRoot, mouse)
            }
        }
        onReleased: {
            root.stopScrolling = false
            parent.y = compositionRoot.parent.height / 2
            var delta = parent.x - startX
            if (Math.abs(delta) >= 1.0 || trackId !== originalTrackId) {
                parent.moved(compositionRoot)
                originalX = parent.x
                originalTrackId = trackId
            } else {
                parent.dropped(compositionRoot)
            }
        }
        onDoubleClicked: timeline.position = compositionRoot.x / timeline.scaleFactor
        onWheel: zoomByWheel(wheel)

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            propagateComposedEvents: true
            cursorShape: (trimInMouseArea.drag.active || trimOutMouseArea.drag.active)? Qt.SizeHorCursor :
                drag.active? Qt.ClosedHandCursor : Qt.OpenHandCursor
            onPressed: {
                root.stopScrolling = true
                compositionRoot.forceActiveFocus();
                if (!compositionRoot.selected) {
                    compositionRoot.clicked(compositionRoot, false)
                }
            }
            onClicked: menu.show()
        }
    }

    Rectangle {
        id: trimIn
        enabled: !isBlank
        anchors.left: parent.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        color: isAudio? 'green' : 'lawngreen'
        opacity: 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis

            onPressed: {
                root.stopScrolling = true
                compositionRoot.originalX = mapToItem(null, x, y).x
                compositionRoot.originalDuration = clipDuration
                parent.anchors.left = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = compositionRoot.left
                compositionRoot.trimmedIn(compositionRoot)
                parent.opacity = 0
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    compositionRoot.draggedX = mapToItem(null, x, y).x
                    var delta = Math.round((draggedX - originalX) / timeScale)
                    if (delta !== 0) {
                        var newDuration = compositionRoot.clipDuration - delta
                        compositionRoot.trimmingIn(compositionRoot, newDuration, mouse)
                    }
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }
    Rectangle {
        id: trimOut
        enabled: !isBlank
        anchors.right: parent.right
        anchors.rightMargin: 0
        height: parent.height
        width: 5
        color: 'red'
        opacity: 0
        Drag.active: trimOutMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction

        MouseArea {
            id: trimOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            drag.target: parent
            drag.axis: Drag.XAxis

            onPressed: {
                root.stopScrolling = true
                compositionRoot.originalDuration = clipDuration
                parent.anchors.right = undefined
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.right = compositionRoot.right
                compositionRoot.trimmedOut(compositionRoot)
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale)
                    compositionRoot.trimmingOut(compositionRoot, newDuration, mouse)
                }
            }
            onEntered: parent.opacity = 0.5
            onExited: parent.opacity = 0
        }
    }
    Menu {
        id: menu
        function show() {
            //mergeItem.visible = timeline.mergeClipWithNext(trackIndex, index, true)
            popup()
        }
        MenuItem {
            visible: true // !isBlank && !isComposition
            text: i18n('Cut')
            onTriggered: {
                if (!trackRoot.isLocked) {
                    timeline.copyClip(trackIndex, index)
                    timeline.remove(trackIndex, index)
                } else {
                    root.pulseLockButtonOnTrack(currentTrack)
                }
            }
        }
        MenuItem {
            visible: !grouped && trackRoot.selection.length > 1
            text: i18n('Group')
            onTriggered: timeline.groupSelection()
        }
        MenuItem {
            visible: grouped
            text: i18n('Ungroup')
            onTriggered: timeline.unGroupSelection(clipId)
        }

        MenuItem {
            visible: true //!isBlank && !isComposition
            text: i18n('Copy')
            onTriggered: timeline.copyClip(trackIndex, index)
        }
        MenuSeparator {
            visible: !isBlank && !isComposition
        }
        MenuItem {
            text: i18n('Remove')
            onTriggered: timeline.triggerAction('delete_timeline_clip')
        }
        MenuItem {
            visible: true //!isBlank
            text: i18n('Lift')
            onTriggered: timeline.lift(trackIndex, index)
        }
        MenuSeparator {
            visible: true //!isBlank && !isComposition
        }
        MenuItem {
            visible: true //!isBlank && !isComposition
            text: i18n('Split At Playhead (S)')
            onTriggered: timeline.splitClip(trackIndex, index)
        }
        MenuItem {
            id: mergeItem
            text: i18n('Merge with next clip')
            onTriggered: timeline.mergeClipWithNext(trackIndex, index, false)
        }
        MenuItem {
            visible: !isBlank && !isComposition
            text: i18n('Rebuild Audio Waveform')
            onTriggered: timeline.remakeAudioLevels(trackIndex, index)
        }
        /*onPopupVisibleChanged: {
            if (visible && application.OS !== 'OS X' && __popupGeometry.height > 0) {
                // Try to fix menu running off screen. This only works intermittently.
                menu.__yOffset = Math.min(0, Screen.height - (__popupGeometry.y + __popupGeometry.height + 40))
                menu.__xOffset = Math.min(0, Screen.width - (__popupGeometry.x + __popupGeometry.width))
            }
        }*/
    }
}