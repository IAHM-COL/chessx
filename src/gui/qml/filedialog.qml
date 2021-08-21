import QtQuick 2.2
import QtQuick.Dialogs 1.3

FileDialog {
    id: fileDialog

    signal selected(url selection, bool utf8)

    folder: shortcuts.home
    sidebarVisible: true
    selectMultiple: false
    selectExisting: true
    property bool utf8flag: false

    onAccepted: {
        console.log("Loaded")
    	fileDialog.selected(fileDialog.fileUrl, fileDialog.utf8flag)
	Qt.quit()
    }
    onRejected: {
        console.log("Not loaded")
        Qt.quit()
    }
}
