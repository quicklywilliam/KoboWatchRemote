import SwiftUI

@main
struct KoboRemoteTestApp: App {
    var body: some Scene {
        WindowGroup {
            ScanTestView()
        }
    }
}

struct ScanTestView: View {
    var ble = BLEManager.shared

    var body: some View {
        VStack(spacing: 16) {
            HStack(spacing: 6) {
                Circle()
                    .fill(ble.state == .connected ? .green :
                          ble.state == .scanning ? .yellow : .red)
                    .frame(width: 12, height: 12)
                Text(ble.state.rawValue)
                    .font(.headline)
            }
            Text(ble.debugInfo)
                .font(.caption)
                .foregroundStyle(.secondary)

            if let action = ble.lastAction {
                Text(action)
                    .font(.title)
                    .foregroundStyle(.green)
            }

            Spacer()

            HStack(spacing: 40) {
                Button {
                    ble.sendPreviousPage()
                } label: {
                    Image(systemName: "chevron.left")
                        .font(.largeTitle)
                        .frame(width: 100, height: 80)
                }
                .buttonStyle(.borderedProminent)
                .tint(.blue)
                .disabled(ble.state != .connected)

                Button {
                    ble.sendNextPage()
                } label: {
                    Image(systemName: "chevron.right")
                        .font(.largeTitle)
                        .frame(width: 100, height: 80)
                }
                .buttonStyle(.borderedProminent)
                .tint(.blue)
                .disabled(ble.state != .connected)
            }

            Spacer()
        }
        .padding()
    }
}
