<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import Light from '~icons/tabler/bulb';
	import Info from '~icons/tabler/info-circle';
	import Save from '~icons/tabler/device-floppy';
	import Reload from '~icons/tabler/reload';
	import { socket } from '$lib/stores/socket';
	import GridForm from '$lib/components/GridForm.svelte';
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import type { ControllerState } from '$lib/types/models'
	import ControllerSettings from '../../lib/components/ControllerSettings.svelte';
	import Select from '$lib/components/Select.svelte';
	import Text from '$lib/components/Text.svelte';
	import Button from '$lib/components/Button.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';
	import DisableButton from '$lib/components/DisableButton.svelte';
	import StopButton from '$lib/components/StopButton.svelte';

	const restPath = "/rest/heliostat";

	let newName = "";

	type Direction = {
		azimuth: number;
		elevation: number;
	}

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
		}
	}

	let heliostatControllerState : HeliostatControllerState;

	let selectedEditor = "Sun";
	let selectedDirection: Direction;
	$: selectedDirection = heliostatControllerState?.sourcesMap[selectedEditor];

	async function getHeliostatControllerState() {
		return getJsonRest(restPath, heliostatControllerState).then((data)=>{heliostatControllerState=data;console.log(heliostatControllerState)});
	}

	async function postHeliostatControllerState() {
		return postJsonRest(restPath, heliostatControllerState).then((data)=>heliostatControllerState=data);
	}

	function syncClientTime() {
		const now = new Date();
		postJsonRest(restPath + '/sunTracker', {
			time: {
				year: now.getUTCFullYear(),
				month: now.getUTCMonth() + 1,
				day: now.getUTCDate(),
				hour: now.getUTCHours(),
				minute: now.getUTCMinutes(),
				second: now.getUTCSeconds(),
			}
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
		<span>Sun Tracker</span>
	{/snippet}
	{#await getHeliostatControllerState() then nothing}
	<div class="alert {heliostatControllerState.sunTracker.isTimeSet ? 'alert-info' : 'alert-warning'} my-2 shadow-lg">
		<Info class="h-6 w-6 flex-shrink-0 stroke-current" />
		<span>
			{#if heliostatControllerState.sunTracker.isTimeSet}
			Azimuth : {heliostatControllerState.sunTracker.azimuth}, Elevation : {heliostatControllerState.sunTracker.elevation}
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