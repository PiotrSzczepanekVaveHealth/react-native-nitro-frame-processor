import { useMemo } from 'react';
import { Text, View, StyleSheet } from 'react-native';
import {
  activateLicense,
  processFrame,
  setEnabled,
  setNumThreads,
  setParameterFilePath,
  setSetting,
} from 'react-native-nitro-frame-processor';

setEnabled(true);
setNumThreads(2);
setSetting(0);
setParameterFilePath('US2D-7_default.us2d7');
const licenseActivated = activateLicense('demo-key', 'demo-device');

export default function App() {
  const outputInfo = useMemo(() => {
    try {
      const width = 128;
      const height = 128;
      const inputBytes = new Uint8Array(width * height);
      for (let i = 0; i < inputBytes.length; i += 1) {
        inputBytes[i] = i % 255;
      }

      const output = processFrame(width, height, inputBytes.buffer);
      const outputBytes = new Uint8Array(output);
      return `Processed ${outputBytes.length} bytes`;
    } catch (error) {
      return `Processing failed: ${String(error)}`;
    }
  }, []);

  return (
    <View style={styles.container}>
      <Text>CVIE Nitro module loaded</Text>
      <Text>License activated: {String(licenseActivated)}</Text>
      <Text>{outputInfo}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
});
