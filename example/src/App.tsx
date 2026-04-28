import { Text, View, StyleSheet } from 'react-native';
import {
  activateLicense,
  setEnabled,
} from 'react-native-nitro-frame-processor';

setEnabled(true);
const licenseActivated = activateLicense('demo-key', 'demo-device');

export default function App() {
  return (
    <View style={styles.container}>
      <Text>CVIE Nitro module loaded</Text>
      <Text>License activated: {String(licenseActivated)}</Text>
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
