<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { slide } from 'svelte/transition';
	import { cubicOut } from 'svelte/easing';
	import SettingsCard from '$lib/components/SettingsCard.svelte';
	import Collapsible from '$lib/components/Collapsible.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import { user } from '$lib/stores/user';
	import { page } from '$app/stores';
	import { notifications } from '$lib/components/toasts/notifications';
	import NTP from '~icons/tabler/clock-check';
	import Server from '~icons/tabler/server';
	import Clock from '~icons/tabler/clock';
	import Satellite from '~icons/tabler/satellite'
	import UTC from '~icons/tabler/clock-pin';
	import Stopwatch from '~icons/tabler/24-hours';
	import type { GPSSettings, GPSStatus } from '$lib/types/models';
	import { socket } from '$lib/stores/socket';
	import Info from '~icons/tabler/info-circle';
	import { Warning } from 'postcss';

	let gpsSettings: GPSSettings;
	let gpsStatus: GPSStatus;
	const gpsStatusEvent = "gps";

	let infoClass;
	$: infoClass = (gpsSettings?.enabled && gpsStatus?.fixQuality == 49 && gpsStatus.hasSerial) ? "alert-info" : "alert-warning";

	onMount(() => {
		socket.on<GPSStatus>(gpsStatusEvent, (data) => {
			gpsStatus = data;
		});
		getGPSSettings();
	});

	onDestroy(() => socket.off(gpsStatusEvent));

	async function getGPSSettings() {
		try {
			const response = await fetch('/rest/gpsSettings', {
				method: 'GET',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				}
			});
			gpsSettings = await response.json();
		} catch (error) {
			console.error('Error:', error);
		}
		return;
	}


	async function postGPSSettings(data: GPSSettings) {
		try {
			const response = await fetch('/rest/gpsSettings', {
				method: 'POST',
				headers: {
					Authorization: $page.data.features.security ? 'Bearer ' + $user.bearer_token : 'Basic',
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(data)
			});

			if (response.status == 200) {
				notifications.success('Security settings updated.', 3000);
				gpsSettings = await response.json();
			} else {
				notifications.error('User not authorized.', 3000);
			}
		} catch (error) {
			console.error('Error:', error);
		}
	}

</script>

<SettingsCard collapsible={false}>
	<Satellite slot="icon" class="lex-shrink-0 mr-2 h-6 w-6 self-end" />
	<span slot="title">GPS</span>

	<div class="alert {infoClass} my-2 shadow-lg">
		<Info class="h-6 w-6 flex-shrink-0 stroke-current" />
		<span>
			{#if gpsStatus}
				{#if gpsSettings?.enabled}
					{#if gpsStatus?.hasSerial && gpsStatus?.numSats > 0}
						Number of satellites: {gpsStatus.numSats} <br>
						Latitude : {gpsStatus.latitude.toFixed(5)},
						Longitude : {gpsStatus.longitude.toFixed(5)},
						Altitude : {gpsStatus.altitude.toFixed(1)} <br>
						Date : {gpsStatus.dateStr}, Time : {gpsStatus.timeStr}
					{:else if gpsStatus?.hasSerial}
						No GPS reception !
					{:else}
						GPS module seems disconnected !
					{/if}
				{:else}
					GPS Disabled
				{/if}
			{/if}
		</span>
	</div>

	{#if !$page.data.features.security || $user.admin}
		<Collapsible open={false} class="shadow-lg" on:closed={getGPSSettings}>
			<span slot="title">Settings</span>
			<form
				class="form-control w-full"
				on:submit|preventDefault={() => postGPSSettings(gpsSettings)}
				novalidate
			>
				<label class="label cursor-pointer justify-start gap-4">
					<input
						type="checkbox"
						bind:checked={gpsSettings.enabled}
						class="checkbox checkbox-primary"
					/>
					<span class="">Enable GPS</span>
				</label>
				<div class="mt-6 place-self-end">
					<button class="btn btn-primary" type="submit">Apply Settings</button>
				</div>
			</form>
		</Collapsible>
	{/if}
</SettingsCard>
