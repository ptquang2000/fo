import QtQuick
import QtQuick.Controls

ApplicationWindow {
	width: 400
	height: 400
	visible: true

	ListView {
		anchors.fill: parent
		model: candidateModel
		delegate: Text {
			text: display
		}
	}
}
