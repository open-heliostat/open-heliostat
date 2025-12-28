<script lang="ts">
	import { onDestroy, onMount } from 'svelte';
	import SettingsCard from './SettingsCard.svelte';
	import Slider from './Slider.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';

	export let azimuthPath = '/rest/heliostat/azimuth';
	export let elevationPath = '/rest/heliostat/elevation';

	type PermissionState = 'unknown' | 'prompt' | 'granted' | 'denied';

	let sensorPossible = false;
	let supported = false;
	let blockedInsecure = false;
	let permission: PermissionState = 'unknown';
	let enabled = false;
	let useMotionFallback = false;

	let beta = 0;
	let gamma = 0;
	let zeroBeta = 0;
	let zeroGamma = 0;
	let azimuthBase = 0;
	let elevationBase = 0;
	let lastSent = 0;

	const throttleMs = 140;
	const maxTilt = 60;
	let sensitivity = 1;

	const hasMotionPermission = ():
		boolean =>
			typeof DeviceMotionEvent !== 'undefined' &&
			typeof (DeviceMotionEvent as typeof DeviceMotionEvent & { requestPermission?: () => Promise<PermissionState> }).requestPermission === 'function';
	const hasOrientationPermission = ():
		boolean =>
			typeof DeviceOrientationEvent !== 'undefined' &&
			typeof (DeviceOrientationEvent as typeof DeviceOrientationEvent & { requestPermission?: () => Promise<PermissionState> }).requestPermission === 'function';

	const clamp = (value: number, min: number, max: number): number => Math.min(Math.max(value, min), max);

	$: previewAz = clamp(azimuthBase + (gamma - zeroGamma) * sensitivity, 0, 360);
	$: previewEl = clamp(elevationBase + (beta - zeroBeta) * sensitivity, 0, 360);

	function handleOrientation(event: DeviceOrientationEvent) {
		beta = event.beta ?? 0;
		gamma = event.gamma ?? 0;
		pushTargets();
	}

	function handleMotion(event: DeviceMotionEvent) {
		const accel = event.accelerationIncludingGravity;
		if (!accel) return;
		const x = accel.x ?? 0;
		const y = accel.y ?? 0;
		const z = accel.z ?? 0;
		// Compute pitch (beta) and roll (gamma) from acceleration.
		beta = (Math.atan2(x, Math.hypot(y, z)) * 180) / Math.PI;
		gamma = (Math.atan2(y, Math.hypot(x, z)) * 180) / Math.PI;
		pushTargets();
	}

	function pushTargets() {
		if (!enabled || permission !== 'granted') return;
		const betaDelta = clamp(beta - zeroBeta, -maxTilt, maxTilt);
		const gammaDelta = clamp(gamma - zeroGamma, -maxTilt, maxTilt);
		const azTarget = clamp(azimuthBase + gammaDelta * sensitivity, 0, 360);
		const elTarget = clamp(elevationBase + betaDelta * sensitivity, 0, 360);
		const now = performance.now();
		if (now - lastSent < throttleMs) return;
		lastSent = now;
		postJsonRest(azimuthPath, { target: azTarget });
		postJsonRest(elevationPath, { target: elTarget });
	}

	async function syncBases() {
		const [az, el] = await Promise.all([
			getJsonRest(azimuthPath + '/position', azimuthBase),
			getJsonRest(elevationPath + '/position', elevationBase)
		]);
		azimuthBase = az ?? 0;
		elevationBase = el ?? 0;
	}

	function calibrateNeutral() {
		zeroBeta = beta;
		zeroGamma = gamma;
	}

	async function requestPermission(): Promise<boolean> {
		if (hasMotionPermission()) {
			try {
				const res = await (DeviceMotionEvent as typeof DeviceMotionEvent & {
					requestPermission?: () => Promise<PermissionState>;
				}).requestPermission?.();
				permission = res === 'granted' ? 'granted' : 'denied';
				return permission === 'granted';
			} catch (error) {
				permission = 'denied';
				console.error('Motion permission error', error);
				return false;
			}
		}
		if (hasOrientationPermission()) {
			try {
				const res = await (DeviceOrientationEvent as typeof DeviceOrientationEvent & {
					requestPermission?: () => Promise<PermissionState>;
				}).requestPermission?.();
				permission = res === 'granted' ? 'granted' : 'denied';
				return permission === 'granted';
			} catch (error) {
				permission = 'denied';
				console.error('Orientation permission error', error);
				return false;
			}
		}
		permission = 'granted';
		return true;
	}

	async function enableTilt() {
		if (!supported) return;
		if (permission !== 'granted') {
			const ok = await requestPermission();
			if (!ok) return;
		}
		await syncBases();
		calibrateNeutral();
		enabled = true;
	}

	function disableTilt() {
		enabled = false;
	}

	onMount(() => {
		const hasWindow = typeof window !== 'undefined';
		const orientationSupported = hasWindow && 'DeviceOrientationEvent' in window;
		const motionSupported = hasWindow && 'DeviceMotionEvent' in window;
		sensorPossible = orientationSupported || motionSupported;
		blockedInsecure = sensorPossible && hasWindow && !window.isSecureContext;
		supported = sensorPossible && !blockedInsecure;
		useMotionFallback = !orientationSupported && motionSupported;
		permission = hasMotionPermission() || hasOrientationPermission() ? 'prompt' : supported ? 'granted' : 'denied';

		if (!supported) return;
		if (useMotionFallback) {
			window.addEventListener('devicemotion', handleMotion, true);
		} else {
			window.addEventListener('deviceorientation', handleOrientation, true);
		}
	});

	onDestroy(() => {
		if (!supported) return;
		if (useMotionFallback) {
			window.removeEventListener('devicemotion', handleMotion, true);
		} else {
			window.removeEventListener('deviceorientation', handleOrientation, true);
		}
	});
</script>

<SettingsCard>
	{#snippet title()}
		<span>Phone Tilt Control</span>
	{/snippet}
	<div class="space-y-4">
		<p class="text-sm opacity-80">
			Tilt your phone to steer azimuth (roll) and elevation (pitch) in real time. Calibrate while the heliostat is
			stationary for best results.
		</p>
		{#if !sensorPossible}
			<div class="alert alert-warning">
				<span>This device does not expose motion/orientation sensors to the browser, so tilt control is unavailable.</span>
			</div>
		{:else if blockedInsecure}
			<div class="alert alert-warning">
				<span>Motion sensors are blocked because the page is not served over HTTPS or localhost. Use a secure origin to enable tilt control.</span>
			</div>
		{:else}
			<div class="flex flex-wrap gap-3">
				{#if permission !== 'granted'}
					<button class="btn btn-primary" on:click={enableTilt}>Allow motion access</button>
				{:else}
					<button class="btn btn-primary" on:click={enabled ? disableTilt : enableTilt}>
						<span>{enabled ? 'Stop tilt control' : 'Start tilt control'}</span>
					</button>
					<button class="btn" disabled={!enabled} on:click={calibrateNeutral}>Re-center</button>
				{/if}
				<button class="btn" disabled={!enabled} on:click={syncBases}>Sync to heliostat</button>
			</div>
			<Slider label="Sensitivity" bind:value={sensitivity} min={0.2} max={2.5} step={0.1}></Slider>
			<div class="grid grid-cols-1 gap-3 sm:grid-cols-2">
				<div class="rounded border border-base-200 p-3">
					<p class="text-xs uppercase opacity-70">Live tilt</p>
					<p class="font-mono text-lg">Pitch {beta.toFixed(1)}° • Roll {gamma.toFixed(1)}°</p>
				</div>
				<div class="rounded border border-base-200 p-3">
					<p class="text-xs uppercase opacity-70">Target preview</p>
					<p class="font-mono text-lg">Az {previewAz.toFixed(1)}° • El {previewEl.toFixed(1)}°</p>
				</div>
			</div>
			{#if useMotionFallback}
				<p class="text-xs opacity-70">Using accelerometer fallback (DeviceMotion).</p>
			{/if}
		{/if}
	</div>
</SettingsCard>
