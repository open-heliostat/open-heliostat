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

	type HeliostatControllerState = {
		azimuth: number;
		elevation: number;
	}

	let heliostatControllerStateEvent = "heliostat-control"

	let heliostatControllerState : HeliostatControllerState;

	onMount(() => {
		socket.on<HeliostatControllerState>(heliostatControllerStateEvent, (data) => {
			heliostatControllerState = data;
		});
	});

	onDestroy(() => {
		socket.off(heliostatControllerStateEvent);
	});

</script>

{#if heliostatControllerState}
<SettingsCard collapsible={false}>
	<Light slot="icon" class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
	<span slot="title">Heliostat Control</span>
	<div class="w-full">
		<Slider 
			label="Azimuth" 
			bind:value={heliostatControllerState.azimuth}
			min={0} 
			max={360} 
			step={0.01}
			onChange={() => socket.sendEvent(heliostatControllerStateEvent, heliostatControllerState)}
		></Slider>
		<Slider 
			label="Elevation" 
			bind:value={heliostatControllerState.elevation}
			min={0} 
			max={360} 
			step={0.01}
			onChange={() => socket.sendEvent(heliostatControllerStateEvent, heliostatControllerState)}
		></Slider>
	</div>
</SettingsCard>
{/if}
