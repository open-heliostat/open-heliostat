<script lang="ts">
	import { onDestroy, onMount } from 'svelte';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import Light from '~icons/tabler/bulb';
	import Info from '~icons/tabler/info-circle';
	import GridForm from '$lib/components/GridForm.svelte';
	import Slider from '$lib/components/Slider.svelte';
	import Select from '$lib/components/Select.svelte';
	import Text from '$lib/components/Text.svelte';
	import Button from '$lib/components/Button.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';
	import DisableButton from '$lib/components/DisableButton.svelte';
	import StopButton from '$lib/components/StopButton.svelte';
	import AccelCalibComp from '$lib/components/AccelCalibComp.svelte';

	const restPath = "/rest/heliostat";

	let newName = "";

	type Direction = {
		azimuth: number;
		elevation: number;
	}

	type MountOrientation = {
		tiltDeg: number;
		tiltAzimuthDeg: number;
	};

	type MountOrientationResolveState = {
		running: boolean;
		useLimits: boolean;
		hasResult: boolean;
		observabilityOk: boolean;
		rms: number;
		completedPoses: number;
		totalPoses: number;
		settleMs: number;
		samplesPerPose: number;
		status: string;
		failureReason: string;
		result: {
			tiltDeg: number;
			tiltAzimuthDeg: number;
			gravityX: number;
			gravityY: number;
			gravityZ: number;
			sensorRollDeg: number;
			sensorPitchDeg: number;
		};
	};

	type HeliostatControllerState = {
		enabled: boolean;
		currentSource: string;
		currentTarget: string;
		sourcesMap: {
			[key: string]: Direction
		}
		sunTracker : {
			latitude: number;
			longitude: number;
			isTimeSet: boolean;
			azimuth: number;
			elevation: number;
			timestamp?: number;
			utcIso?: string;
			localIso?: string;
			tz?: string;
			offsetMinutes?: number;
		};
		mountOrientation: MountOrientation;
	}

	const defaultHeliostatControllerState: HeliostatControllerState = {
		enabled: false,
		currentSource: 'None',
		currentTarget: 'None',
		sourcesMap: {
			Sun: { azimuth: 180, elevation: 45 }
		},
		sunTracker: {
			latitude: 0,
			longitude: 0,
			isTimeSet: false,
			azimuth: 0,
			elevation: 0
		},
		mountOrientation: {
			tiltDeg: 0,
			tiltAzimuthDeg: 0
		}
	};

	let heliostatControllerState : HeliostatControllerState = { ...defaultHeliostatControllerState };
	$: sunTrackerTime = heliostatControllerState?.sunTracker;
	let orientationDraft: MountOrientation = { ...defaultHeliostatControllerState.mountOrientation };
	let orientationDirty = false;
	let orientationApplyStatus = '';
	let orientationResolvePollId: ReturnType<typeof setInterval> | undefined;
	let orientationResolveState: MountOrientationResolveState = {
		running: false,
		useLimits: true,
		hasResult: false,
		observabilityOk: false,
		rms: 0,
		completedPoses: 0,
		totalPoses: 12,
		settleMs: 900,
		samplesPerPose: 10,
		status: 'idle',
		failureReason: '',
		result: {
			tiltDeg: 0,
			tiltAzimuthDeg: 0,
			gravityX: 0,
			gravityY: 0,
			gravityZ: 1,
			sensorRollDeg: 0,
			sensorPitchDeg: 0
		}
	};

	let selectedEditor = "Sun";
	let selectedDirection: Direction;
	$: selectedDirection = heliostatControllerState?.sourcesMap[selectedEditor];

	function getTiltError(value: number): string {
		if (value < -90 || value > 90) {
			return 'Tilt must be between -90 and 90 degrees';
		}
		return '';
	}

	function getTiltAzimuthError(value: number): string {
		if (value < 0 || value > 360) {
			return 'Tilt azimuth must be between 0 and 360 degrees';
		}
		return '';
	}

	$: tiltError = getTiltError(orientationDraft.tiltDeg);
	$: tiltAzimuthError = getTiltAzimuthError(orientationDraft.tiltAzimuthDeg);
	$: hasOrientationErrors = Boolean(tiltError || tiltAzimuthError);
	$: canApplyOrientation = orientationDirty && !hasOrientationErrors;

	function hydrateOrientationDraft() {
		if (orientationDirty) {
			return;
		}

		orientationDraft = {
			tiltDeg: heliostatControllerState.mountOrientation?.tiltDeg ?? 0,
			tiltAzimuthDeg: heliostatControllerState.mountOrientation?.tiltAzimuthDeg ?? 0
		};
	}

	async function getHeliostatControllerState() {
		return getJsonRest(restPath, heliostatControllerState).then((data)=>{
			heliostatControllerState = data;
			hydrateOrientationDraft();
		});
	}

	async function refreshOrientationResolveState() {
		orientationResolveState = await getJsonRest('/rest/heliostat/orientation-resolve', orientationResolveState);
		if (!orientationResolveState.running && orientationResolvePollId) {
			clearInterval(orientationResolvePollId);
			orientationResolvePollId = undefined;
		}
	}

	async function postHeliostatControllerState() {
		return postJsonRest(restPath, heliostatControllerState).then((data)=>heliostatControllerState=data);
	}

	async function applyOrientation() {
		if (!canApplyOrientation) {
			return;
		}

		try {
			await postJsonRest(restPath, {
				mountOrientation: {
					tiltDeg: orientationDraft.tiltDeg,
					tiltAzimuthDeg: orientationDraft.tiltAzimuthDeg
				}
			});

			orientationDirty = false;
			await getHeliostatControllerState();
			orientationApplyStatus = 'Orientation applied successfully';
		} catch (error) {
			orientationApplyStatus = 'Orientation apply failed';
		}
	}

	async function startOrientationResolve() {
		await postJsonRest('/rest/heliostat/orientation-resolve', { running: true });
		await refreshOrientationResolveState();
		if (!orientationResolvePollId) {
			orientationResolvePollId = setInterval(refreshOrientationResolveState, 1000);
		}
	}

	async function stopOrientationResolve() {
		await postJsonRest('/rest/heliostat/orientation-resolve', { running: false });
		await refreshOrientationResolveState();
	}

	async function applyResolvedOrientation() {
		await postJsonRest('/rest/heliostat/orientation-resolve', { apply: true });
		await refreshOrientationResolveState();
		orientationDirty = false;
		await getHeliostatControllerState();
	}

	function useResolvedOrientationDraft() {
		if (!orientationResolveState.hasResult) {
			return;
		}

		orientationDraft = {
			tiltDeg: orientationResolveState.result.tiltDeg,
			tiltAzimuthDeg: orientationResolveState.result.tiltAzimuthDeg
		};
		orientationDirty = true;
		orientationApplyStatus = 'Resolved orientation copied to draft';
	}

	function syncClientTime() {
		const now = new Date();
		postJsonRest(restPath + '/sunTracker', {
			timeIso: now.toISOString().slice(0, 19) // YYYY-MM-DDTHH:MM:SSZ -> strip Z for UTC parsing on device
		}).then(() => getHeliostatControllerState());
	}

	function getBrowserLocation() {
		if (!navigator.geolocation) {
			alert("Geolocation is not supported by your browser");
			return;
		}

		navigator.geolocation.getCurrentPosition(
			(position) => {
				postJsonRest(restPath + '/sunTracker', {
					latitude: position.coords.latitude,
					longitude: position.coords.longitude
				}).then(() => getHeliostatControllerState());
			},
			(error) => {
				alert("Error getting location: " + error.message);
			}
		);
	}

	onMount(() => {
		getHeliostatControllerState();
		refreshOrientationResolveState();
	});

	onDestroy(() => {
		if (orientationResolvePollId) {
			clearInterval(orientationResolvePollId);
		}
	});

</script>

<SettingsCard collapsible={false}>
	{#snippet icon()}
		<Light class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
	{/snippet}
	{#snippet title()}
		<span>Heliostat Control</span>
	{/snippet}
	<div class="w-full">
		{#await getHeliostatControllerState() then nothing}
		<div class="grid w-full grid-cols-1 content-center gap-x-4 sm:grid-cols-2">
			<div>
				<Select label="Source" bind:value={heliostatControllerState.currentSource} onChange={()=>{postJsonRest(restPath, {currentSource: heliostatControllerState.currentSource})}}>
					<option>None</option>
					<option>Sun</option>
					{#each Object.entries(heliostatControllerState?.sourcesMap) as [name, value]}
						<option>{name}</option>
					{/each}
				</Select>
			</div>
			<div>
				<Select label="Target" bind:value={heliostatControllerState.currentTarget} onChange={()=>{postJsonRest(restPath, {currentTarget: heliostatControllerState.currentTarget})}}>
					<option>None</option>
					<option>Sun</option>
					{#each Object.entries(heliostatControllerState?.sourcesMap) as [name, value]}
						<option>{name}</option>
					{/each}
				</Select>
			</div>
		</div>
		<div class="w-full mb-4">
			<Select label="Select" bind:value={selectedEditor} onChange={()=>{newName=selectedEditor}}>
				{#each Object.entries(heliostatControllerState?.sourcesMap) as [name, value]}
					<option>{name}</option>
				{/each}
			</Select>
		</div>
		<GridForm>
			{#if selectedDirection}
			<Text
				label="Name"
				bind:value={newName}
				onChange={()=>{postJsonRest(restPath + "/rename", {oldName: selectedEditor, newName: newName});getHeliostatControllerState().then(()=>selectedEditor=newName)}}
			></Text>
			<Slider 
				label="Azimuth" 
				bind:value={selectedDirection.azimuth}
				min={0} 
				max={360} 
				step={0.01}
				onChange={() => {
					postJsonRest(restPath, {set: {name: selectedEditor, azimuth: selectedDirection.azimuth, elevation: selectedDirection.elevation}});
				}}
			></Slider>
			<Slider 
				label="Elevation" 
				bind:value={selectedDirection.elevation}
				min={0} 
				max={360} 
				step={0.01}
				onChange={() => {
					postJsonRest(restPath, {set: {name: selectedEditor, azimuth: selectedDirection.azimuth, elevation: selectedDirection.elevation}});
				}}
			></Slider>
			{/if}
			<!-- <Checkbox
				label="Enable"
				bind:value={heliostatControllerState.enabled}
			></Checkbox> -->
		</GridForm>
		<div class="flex flex-row flex-wrap justify-between gap-x-2">
			<Button
				label="Add"
				onClick={()=>{postJsonRest(restPath + "/add", {}).then(()=>{getHeliostatControllerState().then(() => selectedEditor = "New target")})}}>
			</Button>
			{#if selectedDirection}
			<Button
				label="Remove"
				onClick={()=>{postJsonRest(restPath, {remove: selectedEditor}).then(()=>{getHeliostatControllerState();})}}>
			</Button>
			{/if}
			<div class="flex-grow"></div>
			<DisableButton onClick={() => postJsonRest(restPath, {azimuth:{enabled: false, stepper: {control: {enabled: false}}},elevation:{enabled: false, stepper: {control: {enabled: false}}}})}></DisableButton>
			<StopButton onClick={() => postJsonRest(restPath, {azimuth:{enabled: false, stepper: {control: {stop: {}}}},elevation:{enabled: false, stepper: {control: {stop: {}}}}})}></StopButton>
		</div>
		{/await}
	</div>
</SettingsCard>

<SettingsCard>
	{#snippet title()}
		<span>Mount Orientation Setup</span>
	{/snippet}
	<div class="w-full">
		<GridForm>
			<Slider
				label="Tilt (deg)"
				bind:value={orientationDraft.tiltDeg}
				min={-90}
				max={90}
				step={0.01}
				strictNumberBounds={false}
				onChange={() => {
					orientationDirty = true;
				}}
			></Slider>
			<div class={tiltError ? 'text-error text-sm' : 'text-base-content/70 text-sm'}>
				Tilt must be between -90 and 90 degrees
			</div>
			<Slider
				label="Tilt Azimuth (deg)"
				bind:value={orientationDraft.tiltAzimuthDeg}
				min={0}
				max={360}
				step={0.01}
				strictNumberBounds={false}
				onChange={() => {
					orientationDirty = true;
				}}
			></Slider>
			<div class={tiltAzimuthError ? 'text-error text-sm' : 'text-base-content/70 text-sm'}>
				Tilt azimuth must be between 0 and 360 degrees
			</div>
		</GridForm>
		<div class="flex flex-row justify-end">
			<button class="btn btn-primary inline-flex items-center" disabled={!canApplyOrientation} on:click={applyOrientation}>
				Apply Orientation
			</button>
		</div>
		{#if orientationApplyStatus}
			<div class="text-info text-sm mt-2">{orientationApplyStatus}</div>
		{/if}
		<div class="alert alert-info mt-4 shadow">
			<span>
				Auto-resolve moves the heliostat through a short guided sequence, estimates installation tilt from gravity, and keeps encoder calibration separate.
			</span>
		</div>
		<div class="mt-4 grid gap-4 md:grid-cols-2">
			<div class="stats shadow">
				<div class="stat">
					<div class="stat-title">Resolve Status</div>
					<div class="stat-value text-lg">{orientationResolveState.status}</div>
					<div class="stat-desc">
						{orientationResolveState.completedPoses}/{orientationResolveState.totalPoses} poses, RMS {orientationResolveState.rms?.toFixed?.(3) ?? '—'}
					</div>
				</div>
			</div>
			<div class="stats shadow">
				<div class="stat">
					<div class="stat-title">Resolved Orientation</div>
					<div class="stat-value text-lg">
						{orientationResolveState.result?.tiltDeg?.toFixed?.(2) ?? '—'}°
					</div>
					<div class="stat-desc">
						Azimuth {orientationResolveState.result?.tiltAzimuthDeg?.toFixed?.(2) ?? '—'}°
					</div>
				</div>
			</div>
		</div>
		{#if orientationResolveState.failureReason}
			<div class="text-error text-sm mt-3">{orientationResolveState.failureReason}</div>
		{/if}
		<div class="mt-4 flex flex-row flex-wrap gap-2">
			<button class="btn btn-secondary" disabled={orientationResolveState.running} on:click={startOrientationResolve}>
				Auto-Resolve Orientation
			</button>
			<button class="btn btn-outline" disabled={!orientationResolveState.running} on:click={stopOrientationResolve}>
				Stop Resolve
			</button>
			<button class="btn" disabled={!orientationResolveState.hasResult} on:click={useResolvedOrientationDraft}>
				Use Result In Draft
			</button>
			<button class="btn btn-success" disabled={!orientationResolveState.hasResult} on:click={applyResolvedOrientation}>
				Apply Resolved Orientation
			</button>
		</div>
	</div>
</SettingsCard>

<SettingsCard>
	{#snippet title()}
		<span>Sun Tracker</span>
	{/snippet}
	{#await getHeliostatControllerState() then nothing}
	<div class="alert {heliostatControllerState.sunTracker.isTimeSet ? 'alert-info' : 'alert-warning'} my-2 shadow-lg">
		<Info class="h-6 w-6 flex-shrink-0 stroke-current" />
		<span>
			{#if heliostatControllerState.sunTracker.isTimeSet}
			Azimuth : {heliostatControllerState.sunTracker.azimuth}, Elevation : {heliostatControllerState.sunTracker.elevation}
			<br />
			Time (UTC): {sunTrackerTime?.utcIso ?? '—'}
			<br />
			Local ({sunTrackerTime?.tz ?? 'UTC'}): {sunTrackerTime?.localIso ?? '—'} {sunTrackerTime?.offsetMinutes !== undefined ? `(offset ${sunTrackerTime.offsetMinutes} min)` : ''}
			{:else}
			Time is not set !
			{/if}
		</span>
	</div>
	<span class="text-lg">Location</span>
	<GridForm>
		<Slider 
		label="Latitude" 
		bind:value={heliostatControllerState.sunTracker.latitude}
		min={0} 
		max={360} 
		step={0.01}
		onChange={postHeliostatControllerState}
		></Slider>
		<Slider 
			label="Longitude" 
			bind:value={heliostatControllerState.sunTracker.longitude}
			min={0} 
			max={360} 
			step={0.01}
			onChange={postHeliostatControllerState}
		></Slider>
	</GridForm>
	<div class="flex flex-row gap-2">
		<Button
			label="Get from GPS"
			onClick={()=>{postJsonRest(restPath + '/sunTracker/getFromGPS', {}).then(() => getHeliostatControllerState())}}>
		</Button>
		<!-- <Button
			label="Get from Browser"
			onClick={getBrowserLocation}>
		</Button> -->
		<div class="flex-grow"></div>
		<Button
			label="Sync Time"
			onClick={syncClientTime}>
		</Button>
	</div>
	{/await}
</SettingsCard>

<AccelCalibComp restPath="/rest/accelcalib"></AccelCalibComp>