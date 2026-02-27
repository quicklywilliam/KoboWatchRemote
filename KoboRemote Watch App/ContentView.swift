import SwiftUI

struct ContentView: View {
    var ble = BLEManager.shared
    var actionState = ActionState.shared
    @State private var crownValue = 0.0
    @State private var lastCrownTick = 0

    var body: some View {
        VStack(spacing: 16) {
            // Connection status
            HStack(spacing: 6) {
                Circle()
                    .fill(statusColor)
                    .frame(width: 8, height: 8)
                Text(ble.state.rawValue)
                    .font(.caption2)
                    .foregroundStyle(.secondary)
            }

            if let action = ble.lastAction {
                Text(action)
                    .font(.title3)
                    .foregroundStyle(.green)
                    .transition(.opacity)
            }

            Spacer()

            // Page turn buttons
            HStack(spacing: 20) {
                Button {
                    ble.sendPreviousPage()
                } label: {
                    Image(systemName: "chevron.left")
                        .font(.title2)
                        .frame(maxWidth: .infinity, minHeight: 50)
                }
                .buttonStyle(.borderedProminent)
                .tint(.blue)
                .disabled(ble.state != .connected)

                Button {
                    ble.sendNextPage()
                } label: {
                    Image(systemName: "chevron.right")
                        .font(.title2)
                        .frame(maxWidth: .infinity, minHeight: 50)
                }
                .buttonStyle(.borderedProminent)
                .tint(.blue)
                .disabled(ble.state != .connected)
            }
        }
        .padding()
        .animation(.easeInOut(duration: 0.2), value: ble.lastAction)
        .focusable()
        .digitalCrownRotation($crownValue, from: -1000, through: 1000, sensitivity: .low, isContinuous: true)
        .onChange(of: actionState.triggered) {
            if actionState.triggered {
                ble.sendNextPage()
            }
        }
        .onChange(of: crownValue) {
            let tick = Int(crownValue)
            if tick != lastCrownTick {
                if tick > lastCrownTick {
                    ble.sendNextPage()
                } else {
                    ble.sendPreviousPage()
                }
                lastCrownTick = tick
            }
        }
    }

    private var statusColor: Color {
        switch ble.state {
        case .connected: .green
        case .scanning, .connecting: .yellow
        case .disconnected: .red
        }
    }
}

#Preview {
    ContentView()
}
